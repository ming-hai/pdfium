// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_STRING_POOL_TEMPLATE_H_
#define CORE_FXCRT_STRING_POOL_TEMPLATE_H_

#include <unordered_set>

#include "core/fxcrt/fx_string.h"

namespace fxcrt {

template <typename StringType>
class StringPoolTemplate {
 public:
  StringType Intern(const StringType& str) { return *pool_.insert(str).first; }
  void Clear() { pool_.clear(); }

 private:
  std::unordered_set<StringType> pool_;
};

extern template class StringPoolTemplate<ByteString>;
extern template class StringPoolTemplate<WideString>;

}  // namespace fxcrt

using fxcrt::StringPoolTemplate;

using ByteStringPool = StringPoolTemplate<ByteString>;
using WideStringPool = StringPoolTemplate<WideString>;

#endif  // CORE_FXCRT_STRING_POOL_TEMPLATE_H_
