// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_ANNOT_H_
#define FXJS_CJS_ANNOT_H_

#include "fpdfsdk/cpdfsdk_baannot.h"
#include "fxjs/cjs_object.h"
#include "fxjs/js_define.h"

class CJS_Annot final : public CJS_Object {
 public:
  static uint32_t GetObjDefnID();
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  CJS_Annot(v8::Local<v8::Object> pObject, CJS_Runtime* pRuntime);
  ~CJS_Annot() override;

  void SetSDKAnnot(CPDFSDK_BAAnnot* annot);

  JS_STATIC_PROP(hidden, hidden, CJS_Annot)
  JS_STATIC_PROP(name, name, CJS_Annot)
  JS_STATIC_PROP(type, type, CJS_Annot)

 private:
  static uint32_t ObjDefnID;
  static const char kName[];
  static const JSPropertySpec PropertySpecs[];

  CJS_Result get_hidden(CJS_Runtime* pRuntime);
  CJS_Result set_hidden(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Result get_name(CJS_Runtime* pRuntime);
  CJS_Result set_name(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  CJS_Result get_type(CJS_Runtime* pRuntime);
  CJS_Result set_type(CJS_Runtime* pRuntime, v8::Local<v8::Value> vp);

  ObservedPtr<CPDFSDK_BAAnnot> annot_;
};

#endif  // FXJS_CJS_ANNOT_H_
