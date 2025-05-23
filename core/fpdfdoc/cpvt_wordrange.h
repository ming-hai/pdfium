// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPVT_WORDRANGE_H_
#define CORE_FPDFDOC_CPVT_WORDRANGE_H_

#include <algorithm>
#include <utility>

#include "core/fpdfdoc/cpvt_wordplace.h"

struct CPVT_WordRange {
  CPVT_WordRange() = default;

  CPVT_WordRange(const CPVT_WordPlace& begin, const CPVT_WordPlace& end)
      : BeginPos(begin), EndPos(end) {
    Normalize();
  }

  friend inline bool operator==(const CPVT_WordRange& lhs,
                                const CPVT_WordRange& rhs) {
    return lhs.BeginPos == rhs.BeginPos && lhs.EndPos == rhs.EndPos;
  }

  inline bool IsEmpty() const { return BeginPos == EndPos; }

  void Normalize() {
    if (BeginPos > EndPos) {
      std::swap(BeginPos, EndPos);
    }
  }

  CPVT_WordPlace BeginPos;
  CPVT_WordPlace EndPos;
};

#endif  // CORE_FPDFDOC_CPVT_WORDRANGE_H_
