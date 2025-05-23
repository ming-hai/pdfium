// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "core/fpdfapi/page/cpdf_psengine.h"
#include "core/fxcrt/span.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  CPDF_PSEngine engine;
  if (engine.Parse(pdfium::span(data, size))) {
    engine.Execute();
  }
  return 0;
}
