// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_ffnumericedit.h"

#include "core/fxcrt/check.h"
#include "xfa/fwl/cfwl_edit.h"
#include "xfa/fwl/cfwl_eventvalidate.h"
#include "xfa/fwl/cfwl_notedriver.h"
#include "xfa/fxfa/cxfa_ffdoc.h"
#include "xfa/fxfa/parser/cxfa_localevalue.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/xfa_utils.h"

CXFA_FFNumericEdit::CXFA_FFNumericEdit(CXFA_Node* pNode)
    : CXFA_FFTextEdit(pNode) {}

CXFA_FFNumericEdit::~CXFA_FFNumericEdit() = default;

bool CXFA_FFNumericEdit::LoadWidget() {
  DCHECK(!IsLoaded());

  CFWL_Edit* pWidget = cppgc::MakeGarbageCollected<CFWL_Edit>(
      GetFWLApp()->GetHeap()->GetAllocationHandle(), GetFWLApp(),
      CFWL_Widget::Properties(), nullptr);
  SetNormalWidget(pWidget);
  pWidget->SetAdapterIface(this);

  CFWL_NoteDriver* pNoteDriver = pWidget->GetFWLApp()->GetNoteDriver();
  pNoteDriver->RegisterEventTarget(pWidget, pWidget);
  old_delegate_ = pWidget->GetDelegate();
  pWidget->SetDelegate(this);

  {
    CFWL_Widget::ScopedUpdateLock update_lock(pWidget);
    pWidget->SetText(node_->GetValue(XFA_ValuePicture::kDisplay));
    UpdateWidgetProperty();
  }

  return CXFA_FFField::LoadWidget();
}

void CXFA_FFNumericEdit::UpdateWidgetProperty() {
  CFWL_Edit* pWidget = static_cast<CFWL_Edit*>(GetNormalWidget());
  if (!pWidget) {
    return;
  }

  uint32_t dwExtendedStyle =
      FWL_STYLEEXT_EDT_ShowScrollbarFocus | FWL_STYLEEXT_EDT_OuterScrollbar |
      FWL_STYLEEXT_EDT_Validate | FWL_STYLEEXT_EDT_Number;
  dwExtendedStyle |= UpdateUIProperty();
  if (!node_->IsHorizontalScrollPolicyOff()) {
    dwExtendedStyle |= FWL_STYLEEXT_EDT_AutoHScroll;
  }

  std::optional<int32_t> numCells = node_->GetNumberOfCells();
  if (numCells.has_value() && numCells.value() > 0) {
    dwExtendedStyle |= FWL_STYLEEXT_EDT_CombText;
    pWidget->SetLimit(numCells.value());
  }
  dwExtendedStyle |= GetAlignment();
  if (!node_->IsOpenAccess() || !GetDoc()->GetXFADoc()->IsInteractive()) {
    dwExtendedStyle |= FWL_STYLEEXT_EDT_ReadOnly;
  }

  GetNormalWidget()->ModifyStyleExts(dwExtendedStyle, 0xFFFFFFFF);
}

void CXFA_FFNumericEdit::OnProcessEvent(CFWL_Event* pEvent) {
  if (pEvent->GetType() == CFWL_Event::Type::Validate) {
    CFWL_EventValidate* event = static_cast<CFWL_EventValidate*>(pEvent);
    event->SetValidate(OnValidate(GetNormalWidget(), event->GetInsert()));
    return;
  }
  CXFA_FFTextEdit::OnProcessEvent(pEvent);
}

bool CXFA_FFNumericEdit::OnValidate(CFWL_Widget* pWidget,
                                    const WideString& wsText) {
  WideString wsPattern = node_->GetPictureContent(XFA_ValuePicture::kEdit);
  if (!wsPattern.IsEmpty()) {
    return true;
  }

  WideString wsFormat;
  CXFA_LocaleValue widgetValue = XFA_GetLocaleValue(node_.Get());
  widgetValue.GetNumericFormat(wsFormat, node_->GetLeadDigits(),
                               node_->GetFracDigits());
  return widgetValue.ValidateNumericTemp(wsText, wsFormat, node_->GetLocale());
}
