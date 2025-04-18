// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_ANDROID_CFPF_SKIADEVICEMODULE_H_
#define CORE_FXGE_ANDROID_CFPF_SKIADEVICEMODULE_H_

#include <memory>

class CFPF_SkiaFontMgr;

class CFPF_SkiaDeviceModule {
 public:
  CFPF_SkiaDeviceModule();
  ~CFPF_SkiaDeviceModule();

  void Destroy();
  CFPF_SkiaFontMgr* GetFontMgr();

 protected:
  std::unique_ptr<CFPF_SkiaFontMgr> font_mgr_;
};

CFPF_SkiaDeviceModule* CFPF_GetSkiaDeviceModule();

#endif  // CORE_FXGE_ANDROID_CFPF_SKIADEVICEMODULE_H_
