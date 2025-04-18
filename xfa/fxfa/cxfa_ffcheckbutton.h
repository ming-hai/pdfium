// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFCHECKBUTTON_H_
#define XFA_FXFA_CXFA_FFCHECKBUTTON_H_

#include "v8/include/cppgc/member.h"
#include "xfa/fxfa/cxfa_fffield.h"
#include "xfa/fxfa/cxfa_ffpageview.h"
#include "xfa/fxfa/parser/cxfa_node.h"

class CXFA_CheckButton;

class CXFA_FFCheckButton final : public CXFA_FFField {
 public:
  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;
  ~CXFA_FFCheckButton() override;

  void Trace(cppgc::Visitor* visitor) const override;

  // CXFA_FFField
  void RenderWidget(CFGAS_GEGraphics* pGS,
                    const CFX_Matrix& matrix,
                    HighlightOption highlight) override;

  bool LoadWidget() override;
  void PerformLayout() override;
  void UpdateFWLData() override;
  void UpdateWidgetProperty() override;
  bool OnLButtonUp(Mask<XFA_FWL_KeyFlag> dwFlags,
                   const CFX_PointF& point) override;
  void OnProcessMessage(CFWL_Message* pMessage) override;
  void OnProcessEvent(pdfium::CFWL_Event* pEvent) override;
  void OnDrawWidget(CFGAS_GEGraphics* pGraphics,
                    const CFX_Matrix& matrix) override;
  FormFieldType GetFormFieldType() override;

  void SetFWLCheckState(XFA_CheckState eCheckState);

 private:
  CXFA_FFCheckButton(CXFA_Node* pNode, CXFA_CheckButton* button);

  bool CommitData() override;
  bool IsDataChanged() override;
  void CapLeftRightPlacement(const CXFA_Margin* captionMargin);
  void AddUIMargin(XFA_AttributeValue iCapPlacement);
  XFA_CheckState FWLState2XFAState();

  cppgc::Member<IFWL_WidgetDelegate> old_delegate_;
  cppgc::Member<CXFA_CheckButton> const button_;
  CFX_RectF check_box_rect_;
};

#endif  // XFA_FXFA_CXFA_FFCHECKBUTTON_H_
