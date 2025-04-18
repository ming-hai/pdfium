// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/xfa_utils.h"

#include <algorithm>
#include <vector>

#include "core/fxcrt/cfx_memorystream.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/widetext_buffer.h"
#include "core/fxcrt/xml/cfx_xmlchardata.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlnode.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "fxjs/xfa/cjx_object.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_localemgr.h"
#include "xfa/fxfa/parser/cxfa_measurement.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_ui.h"
#include "xfa/fxfa/parser/cxfa_value.h"
#include "xfa/fxfa/parser/xfa_basic_data.h"

namespace {

const char kFormNS[] = "http://www.xfa.org/schema/xfa-form/";

WideString ExportEncodeAttribute(const WideString& str) {
  WideString textBuf;
  textBuf.Reserve(str.GetLength());  // Result always at least as big as input.
  for (size_t i = 0; i < str.GetLength(); i++) {
    switch (str[i]) {
      case '&':
        textBuf += L"&amp;";
        break;
      case '<':
        textBuf += L"&lt;";
        break;
      case '>':
        textBuf += L"&gt;";
        break;
      case '\'':
        textBuf += L"&apos;";
        break;
      case '\"':
        textBuf += L"&quot;";
        break;
      default:
        textBuf += str[i];
    }
  }
  return textBuf;
}

bool IsXMLValidChar(wchar_t ch) {
  return ch == 0x09 || ch == 0x0A || ch == 0x0D ||
         (ch >= 0x20 && ch <= 0xD7FF) || (ch >= 0xE000 && ch <= 0xFFFD);
}

WideString ExportEncodeContent(const WideString& str) {
  WideTextBuffer textBuf;
  size_t iLen = str.GetLength();
  for (size_t i = 0; i < iLen; i++) {
    wchar_t ch = str[i];
    if (!IsXMLValidChar(ch)) {
      continue;
    }

    if (ch == '&') {
      textBuf << "&amp;";
    } else if (ch == '<') {
      textBuf << "&lt;";
    } else if (ch == '>') {
      textBuf << "&gt;";
    } else if (ch == '\'') {
      textBuf << "&apos;";
    } else if (ch == '\"') {
      textBuf << "&quot;";
    } else if (ch == ' ') {
      if (i && str[i - 1] != ' ') {
        textBuf.AppendChar(' ');
      } else {
        textBuf << "&#x20;";
      }
    } else {
      textBuf.AppendChar(str[i]);
    }
  }
  return textBuf.MakeString();
}

bool AttributeSaveInDataModel(CXFA_Node* pNode, XFA_Attribute eAttribute) {
  bool bSaveInDataModel = false;
  if (pNode->GetElementType() != XFA_Element::Image) {
    return bSaveInDataModel;
  }

  CXFA_Node* pValueNode = pNode->GetParent();
  if (!pValueNode || pValueNode->GetElementType() != XFA_Element::Value) {
    return bSaveInDataModel;
  }

  CXFA_Node* pFieldNode = pValueNode->GetParent();
  if (pFieldNode && pFieldNode->GetBindData() &&
      eAttribute == XFA_Attribute::Href) {
    bSaveInDataModel = true;
  }
  return bSaveInDataModel;
}

bool ContentNodeNeedtoExport(CXFA_Node* pContentNode) {
  std::optional<WideString> wsContent =
      pContentNode->JSObject()->TryContent(false, false);
  if (!wsContent.has_value()) {
    return false;
  }

  DCHECK(pContentNode->IsContentNode());
  CXFA_Node* pParentNode = pContentNode->GetParent();
  if (!pParentNode || pParentNode->GetElementType() != XFA_Element::Value) {
    return true;
  }

  CXFA_Node* pGrandParentNode = pParentNode->GetParent();
  if (!pGrandParentNode || !pGrandParentNode->IsContainerNode()) {
    return true;
  }
  if (!pGrandParentNode->GetBindData()) {
    return false;
  }
  if (pGrandParentNode->GetFFWidgetType() == XFA_FFWidgetType::kPasswordEdit) {
    return false;
  }
  return true;
}

WideString SaveAttribute(CXFA_Node* pNode,
                         XFA_Attribute eName,
                         WideStringView wsName,
                         bool bProto) {
  if (!bProto && !pNode->JSObject()->HasAttribute(eName)) {
    return WideString();
  }

  std::optional<WideString> value =
      pNode->JSObject()->TryAttribute(eName, false);
  if (!value.has_value()) {
    return WideString();
  }

  WideString wsEncoded = ExportEncodeAttribute(value.value());
  return WideString{L" ", wsName, L"=\"", wsEncoded.AsStringView(), L"\""};
}

void RegenerateFormFile_Changed(CXFA_Node* pNode,
                                WideTextBuffer& buf,
                                bool bSaveXML) {
  WideString wsAttrs;
  for (size_t i = 0;; ++i) {
    XFA_Attribute attr = pNode->GetAttribute(i);
    if (attr == XFA_Attribute::Unknown) {
      break;
    }

    if (attr == XFA_Attribute::Name ||
        (AttributeSaveInDataModel(pNode, attr) && !bSaveXML)) {
      continue;
    }
    WideString wsAttr = WideString::FromASCII(XFA_AttributeToName(attr));
    wsAttrs += SaveAttribute(pNode, attr, wsAttr.AsStringView(), bSaveXML);
  }

  WideString wsChildren;
  switch (pNode->GetObjectType()) {
    case XFA_ObjectType::ContentNode: {
      if (!bSaveXML && !ContentNodeNeedtoExport(pNode)) {
        break;
      }

      CXFA_Node* pRawValueNode = pNode->GetFirstChild();
      while (pRawValueNode &&
             pRawValueNode->GetElementType() != XFA_Element::SharpxHTML &&
             pRawValueNode->GetElementType() != XFA_Element::Sharptext &&
             pRawValueNode->GetElementType() != XFA_Element::Sharpxml) {
        pRawValueNode = pRawValueNode->GetNextSibling();
      }
      if (!pRawValueNode) {
        break;
      }

      std::optional<WideString> contentType =
          pNode->JSObject()->TryAttribute(XFA_Attribute::ContentType, false);
      if (pRawValueNode->GetElementType() == XFA_Element::SharpxHTML &&
          contentType.has_value() &&
          contentType.value().EqualsASCII("text/html")) {
        CFX_XMLNode* pExDataXML = pNode->GetXMLMappingNode();
        if (!pExDataXML) {
          break;
        }

        CFX_XMLNode* pRichTextXML = pExDataXML->GetFirstChild();
        if (!pRichTextXML) {
          break;
        }

        auto pMemStream = pdfium::MakeRetain<CFX_MemoryStream>();
        pRichTextXML->Save(pMemStream);
        wsChildren +=
            WideString::FromUTF8(ByteStringView(pMemStream->GetSpan()));
      } else if (pRawValueNode->GetElementType() == XFA_Element::Sharpxml &&
                 contentType.has_value() &&
                 contentType.value().EqualsASCII("text/xml")) {
        std::optional<WideString> rawValue =
            pRawValueNode->JSObject()->TryAttribute(XFA_Attribute::Value,
                                                    false);
        if (!rawValue.has_value() || rawValue->IsEmpty()) {
          break;
        }

        std::vector<WideString> wsSelTextArray =
            fxcrt::Split(rawValue.value(), L'\n');

        CXFA_Node* pParentNode = pNode->GetParent();
        CXFA_Node* pGrandparentNode = pParentNode->GetParent();
        WideString bodyTagName =
            pGrandparentNode->JSObject()->GetCData(XFA_Attribute::Name);
        if (bodyTagName.IsEmpty()) {
          bodyTagName = L"ListBox1";
        }

        buf << "<" << bodyTagName << " xmlns=\"\">\n";
        for (const WideString& text : wsSelTextArray) {
          buf << "<value>" << ExportEncodeContent(text) << "</value>\n";
        }
        buf << "</" << bodyTagName << ">\n";
        wsChildren += buf.AsStringView();
        buf.Clear();
      } else {
        WideString wsValue =
            pRawValueNode->JSObject()->GetCData(XFA_Attribute::Value);
        wsChildren += ExportEncodeContent(wsValue);
      }
      break;
    }
    case XFA_ObjectType::TextNode:
    case XFA_ObjectType::NodeC:
    case XFA_ObjectType::NodeV: {
      WideString wsValue = pNode->JSObject()->GetCData(XFA_Attribute::Value);
      wsChildren += ExportEncodeContent(wsValue);
      break;
    }
    default:
      if (pNode->GetElementType() == XFA_Element::Items) {
        CXFA_Node* pTemplateNode = pNode->GetTemplateNodeIfExists();
        if (!pTemplateNode ||
            pTemplateNode->CountChildren(XFA_Element::Unknown, false) !=
                pNode->CountChildren(XFA_Element::Unknown, false)) {
          bSaveXML = true;
        }
      }
      WideTextBuffer newBuf;
      CXFA_Node* pChildNode = pNode->GetFirstChild();
      while (pChildNode) {
        RegenerateFormFile_Changed(pChildNode, newBuf, bSaveXML);
        wsChildren += newBuf.AsStringView();
        newBuf.Clear();
        pChildNode = pChildNode->GetNextSibling();
      }
      if (!bSaveXML && !wsChildren.IsEmpty() &&
          pNode->GetElementType() == XFA_Element::Items) {
        wsChildren.clear();
        bSaveXML = true;
        CXFA_Node* pChild = pNode->GetFirstChild();
        while (pChild) {
          RegenerateFormFile_Changed(pChild, newBuf, bSaveXML);
          wsChildren += newBuf.AsStringView();
          newBuf.Clear();
          pChild = pChild->GetNextSibling();
        }
      }
      break;
  }

  if (!wsChildren.IsEmpty() || !wsAttrs.IsEmpty() ||
      pNode->JSObject()->HasAttribute(XFA_Attribute::Name)) {
    WideString wsElement = WideString::FromASCII(pNode->GetClassName());
    buf << "<";
    buf << wsElement;
    buf << SaveAttribute(pNode, XFA_Attribute::Name, L"name", true);
    buf << wsAttrs;
    if (wsChildren.IsEmpty()) {
      buf << "/>\n";
    } else {
      buf << ">\n" << wsChildren << "</" << wsElement << ">\n";
    }
  }
}

void RegenerateFormFile_Container(CXFA_Node* pNode,
                                  const RetainPtr<IFX_SeekableStream>& pStream,
                                  bool bSaveXML) {
  XFA_Element eType = pNode->GetElementType();
  if (eType == XFA_Element::Field || eType == XFA_Element::Draw ||
      !pNode->IsContainerNode()) {
    WideTextBuffer buf;
    RegenerateFormFile_Changed(pNode, buf, bSaveXML);
    size_t nLen = buf.GetLength();
    if (nLen > 0) {
      pStream->WriteString(buf.MakeString().ToUTF8().AsStringView());
    }
    return;
  }

  WideString wsElement = WideString::FromASCII(pNode->GetClassName());
  pStream->WriteString("<");
  pStream->WriteString(wsElement.ToUTF8().AsStringView());

  WideString wsOutput =
      SaveAttribute(pNode, XFA_Attribute::Name, L"name", true);
  for (size_t i = 0;; ++i) {
    XFA_Attribute attr = pNode->GetAttribute(i);
    if (attr == XFA_Attribute::Unknown) {
      break;
    }
    if (attr == XFA_Attribute::Name) {
      continue;
    }

    WideString wsAttr = WideString::FromASCII(XFA_AttributeToName(attr));
    wsOutput += SaveAttribute(pNode, attr, wsAttr.AsStringView(), false);
  }

  if (!wsOutput.IsEmpty()) {
    pStream->WriteString(wsOutput.ToUTF8().AsStringView());
  }

  CXFA_Node* pChildNode = pNode->GetFirstChild();
  if (!pChildNode) {
    pStream->WriteString(" />\n");
    return;
  }

  pStream->WriteString(">\n");
  while (pChildNode) {
    RegenerateFormFile_Container(pChildNode, pStream, bSaveXML);
    pChildNode = pChildNode->GetNextSibling();
  }
  pStream->WriteString("</");
  pStream->WriteString(wsElement.ToUTF8().AsStringView());
  pStream->WriteString(">\n");
}

WideString RecognizeXFAVersionNumber(CXFA_Node* pTemplateRoot) {
  if (!pTemplateRoot) {
    return WideString();
  }

  std::optional<WideString> templateNS =
      pTemplateRoot->JSObject()->TryNamespace();
  if (!templateNS.has_value()) {
    return WideString();
  }

  XFA_VERSION eVersion =
      pTemplateRoot->GetDocument()->RecognizeXFAVersionNumber(
          templateNS.value());
  if (eVersion == XFA_VERSION_UNKNOWN) {
    eVersion = XFA_VERSION_DEFAULT;
  }

  return WideString::Format(L"%i.%i", eVersion / 100, eVersion % 100);
}

}  // namespace

CXFA_LocaleValue XFA_GetLocaleValue(const CXFA_Node* pNode) {
  const auto* pNodeValue =
      pNode->GetChild<CXFA_Value>(0, XFA_Element::Value, false);
  if (!pNodeValue) {
    return CXFA_LocaleValue();
  }

  CXFA_Node* pValueChild = pNodeValue->GetFirstChild();
  if (!pValueChild) {
    return CXFA_LocaleValue();
  }

  return CXFA_LocaleValue(XFA_GetLocaleValueType(pValueChild->GetElementType()),
                          pNode->GetRawValue(),
                          pNode->GetDocument()->GetLocaleMgr());
}

CXFA_LocaleValue::ValueType XFA_GetLocaleValueType(XFA_Element element) {
  switch (element) {
    case XFA_Element::Decimal:
      return CXFA_LocaleValue::ValueType::kDecimal;
    case XFA_Element::Float:
      return CXFA_LocaleValue::ValueType::kFloat;
    case XFA_Element::Date:
      return CXFA_LocaleValue::ValueType::kDate;
    case XFA_Element::Time:
      return CXFA_LocaleValue::ValueType::kTime;
    case XFA_Element::DateTime:
      return CXFA_LocaleValue::ValueType::kDateTime;
    case XFA_Element::Boolean:
      return CXFA_LocaleValue::ValueType::kBoolean;
    case XFA_Element::Integer:
      return CXFA_LocaleValue::ValueType::kInteger;
    case XFA_Element::Text:
      return CXFA_LocaleValue::ValueType::kText;
    default:
      return CXFA_LocaleValue::ValueType::kNull;
  }
}

bool XFA_FDEExtension_ResolveNamespaceQualifier(CFX_XMLElement* pNode,
                                                const WideString& wsQualifier,
                                                WideString* wsNamespaceURI) {
  if (!pNode) {
    return false;
  }

  CFX_XMLNode* pFakeRoot = pNode->GetRoot();
  WideString wsNSAttribute;
  bool bRet = false;
  if (wsQualifier.IsEmpty()) {
    wsNSAttribute = L"xmlns";
    bRet = true;
  } else {
    wsNSAttribute = L"xmlns:" + wsQualifier;
  }
  for (CFX_XMLNode* pParent = pNode; pParent != pFakeRoot;
       pParent = pParent->GetParent()) {
    CFX_XMLElement* pElement = ToXMLElement(pParent);
    if (pElement && pElement->HasAttribute(wsNSAttribute)) {
      *wsNamespaceURI = pElement->GetAttribute(wsNSAttribute);
      return true;
    }
  }
  wsNamespaceURI->clear();
  return bRet;
}

void XFA_DataExporter_DealWithDataGroupNode(CXFA_Node* pDataNode) {
  if (!pDataNode || pDataNode->GetElementType() == XFA_Element::DataValue) {
    return;
  }

  int32_t iChildNum = 0;
  for (CXFA_Node* pChildNode = pDataNode->GetFirstChild(); pChildNode;
       pChildNode = pChildNode->GetNextSibling()) {
    iChildNum++;
    XFA_DataExporter_DealWithDataGroupNode(pChildNode);
  }

  if (pDataNode->GetElementType() != XFA_Element::DataGroup) {
    return;
  }

  CFX_XMLElement* pElement = ToXMLElement(pDataNode->GetXMLMappingNode());
  if (iChildNum > 0) {
    pElement->RemoveAttribute(L"xfa:dataNode");
    return;
  }
  pElement->SetAttribute(L"xfa:dataNode", L"dataGroup");
}

void XFA_DataExporter_RegenerateFormFile(
    CXFA_Node* pNode,
    const RetainPtr<IFX_SeekableStream>& pStream,
    bool bSaveXML) {
  if (pNode->IsModelNode()) {
    pStream->WriteString("<form xmlns=\"");
    pStream->WriteString(kFormNS);

    WideString wsVersionNumber = RecognizeXFAVersionNumber(
        ToNode(pNode->GetDocument()->GetXFAObject(XFA_HASHCODE_Template)));
    if (wsVersionNumber.IsEmpty()) {
      wsVersionNumber = L"2.8";
    }

    wsVersionNumber += L"/\">\n";
    pStream->WriteString(wsVersionNumber.ToUTF8().AsStringView());

    CXFA_Node* pChildNode = pNode->GetFirstChild();
    while (pChildNode) {
      RegenerateFormFile_Container(pChildNode, pStream, false);
      pChildNode = pChildNode->GetNextSibling();
    }
    pStream->WriteString("</form>\n");
  } else {
    RegenerateFormFile_Container(pNode, pStream, bSaveXML);
  }
}

bool XFA_FieldIsMultiListBox(const CXFA_Node* pFieldNode) {
  if (!pFieldNode) {
    return false;
  }

  const auto* pUIChild =
      pFieldNode->GetChild<CXFA_Ui>(0, XFA_Element::Ui, false);
  if (!pUIChild) {
    return false;
  }

  CXFA_Node* pFirstChild = pUIChild->GetFirstChild();
  if (!pFirstChild ||
      pFirstChild->GetElementType() != XFA_Element::ChoiceList) {
    return false;
  }

  return pFirstChild->JSObject()->GetEnum(XFA_Attribute::Open) ==
         XFA_AttributeValue::MultiSelect;
}

int32_t XFA_MapRotation(int32_t nRotation) {
  nRotation = nRotation % 360;
  nRotation = nRotation < 0 ? nRotation + 360 : nRotation;
  return nRotation;
}

void XFA_EventErrorAccumulate(XFA_EventError* pAcc, XFA_EventError eNew) {
  if (*pAcc == XFA_EventError::kNotExist || eNew == XFA_EventError::kError) {
    *pAcc = eNew;
  }
}
