// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_MESSAGEKILLFOCUS_H_
#define XFA_FWL_CFWL_MESSAGEKILLFOCUS_H_

#include "core/fxcrt/unowned_ptr.h"
#include "xfa/fwl/cfwl_message.h"

namespace pdfium {

class CFWL_MessageKillFocus final : public CFWL_Message {
 public:
  explicit CFWL_MessageKillFocus(CFWL_Widget* pDstTarget);
  ~CFWL_MessageKillFocus() override;

  bool IsFocusedOnWidget(const CFWL_Widget* pWidget) const {
    return pWidget == set_focus_;
  }

 private:
  UnownedPtr<CFWL_Widget> set_focus_;  // Ok, stack-only.
};

}  // namespace pdfium

// TODO(crbug.com/42271761): Remove.
using pdfium::CFWL_MessageKillFocus;

#endif  // XFA_FWL_CFWL_MESSAGEKILLFOCUS_H_
