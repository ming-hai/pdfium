// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_JBIG2_JBIG2_ARITHINTDECODER_H_
#define CORE_FXCODEC_JBIG2_JBIG2_ARITHINTDECODER_H_

#include <stdint.h>

#include <vector>

#include "core/fxcodec/jbig2/JBig2_ArithDecoder.h"

class CJBig2_ArithIntDecoder {
 public:
  CJBig2_ArithIntDecoder();
  ~CJBig2_ArithIntDecoder();

  // Returns true on success, and false when an OOB condition occurs. Many
  // callers can tolerate OOB and do not check the return value.
  bool Decode(CJBig2_ArithDecoder* pArithDecoder, int* nResult);

 private:
  std::vector<JBig2ArithCtx> iax_;
};

class CJBig2_ArithIaidDecoder {
 public:
  explicit CJBig2_ArithIaidDecoder(unsigned char SBSYMCODELENA);
  ~CJBig2_ArithIaidDecoder();

  void Decode(CJBig2_ArithDecoder* pArithDecoder, uint32_t* nResult);

 private:
  std::vector<JBig2ArithCtx> iaid_;

  const unsigned char SBSYMCODELEN;
};

#endif  // CORE_FXCODEC_JBIG2_JBIG2_ARITHINTDECODER_H_
