// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CFX_GLOBALDATA_H_
#define FXJS_CFX_GLOBALDATA_H_

#include <memory>
#include <optional>
#include <vector>

#include "core/fxcrt/binary_buffer.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"
#include "fxjs/cfx_keyvalue.h"

class CFX_GlobalData {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual bool StoreBuffer(pdfium::span<const uint8_t> pBuffer) = 0;
    virtual std::optional<pdfium::span<uint8_t>> LoadBuffer() = 0;
    virtual void BufferDone() = 0;
  };

  class Element {
   public:
    Element();
    ~Element();

    CFX_KeyValue data;
    bool bPersistent = false;
  };

  static CFX_GlobalData* GetRetainedInstance(Delegate* pDelegate);
  bool Release();

  void SetGlobalVariableNumber(ByteString propname, double dData);
  void SetGlobalVariableBoolean(ByteString propname, bool bData);
  void SetGlobalVariableString(ByteString propname, const ByteString& sData);
  void SetGlobalVariableObject(
      ByteString propname,
      std::vector<std::unique_ptr<CFX_KeyValue>> array);
  void SetGlobalVariableNull(ByteString propname);
  bool SetGlobalVariablePersistent(ByteString propname, bool bPersistent);
  bool DeleteGlobalVariable(ByteString propname);

  int32_t GetSize() const;
  Element* GetAt(int index);

  // Exposed for testing.
  Element* GetGlobalVariable(const ByteString& sPropname);

 private:
  using iterator = std::vector<std::unique_ptr<Element>>::iterator;

  explicit CFX_GlobalData(Delegate* pDelegate);
  ~CFX_GlobalData();

  bool LoadGlobalPersistentVariables();
  bool LoadGlobalPersistentVariablesFromBuffer(pdfium::span<uint8_t> buffer);
  bool SaveGlobalPersisitentVariables();
  iterator FindGlobalVariable(const ByteString& sPropname);

  size_t ref_count_ = 0;
  UnownedPtr<Delegate> const delegate_;
  std::vector<std::unique_ptr<Element>> array_global_data_;
};

#endif  // FXJS_CFX_GLOBALDATA_H_
