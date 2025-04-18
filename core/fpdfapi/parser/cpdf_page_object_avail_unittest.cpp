// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/cpdf_page_object_avail.h"

#include <map>
#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_indirect_object_holder.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_read_validator.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_stream.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/invalid_seekable_read_stream.h"

namespace {

class TestReadValidator final : public CPDF_ReadValidator {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;

  void SimulateReadError() { ReadBlockAtOffset({}, 0); }

 private:
  TestReadValidator()
      : CPDF_ReadValidator(pdfium::MakeRetain<InvalidSeekableReadStream>(100),
                           nullptr) {}
  ~TestReadValidator() override = default;
};

class TestHolder final : public CPDF_IndirectObjectHolder {
 public:
  enum class ObjectState {
    Unavailable,
    Available,
  };
  TestHolder() : validator_(pdfium::MakeRetain<TestReadValidator>()) {}
  ~TestHolder() override = default;

  // CPDF_IndirectObjectHolder overrides:
  RetainPtr<CPDF_Object> ParseIndirectObject(uint32_t objnum) override {
    auto it = objects_data_.find(objnum);
    if (it == objects_data_.end()) {
      return nullptr;
    }

    ObjectData& obj_data = it->second;
    if (obj_data.state == ObjectState::Unavailable) {
      validator_->SimulateReadError();
      return nullptr;
    }
    return obj_data.object;
  }

  RetainPtr<CPDF_ReadValidator> GetValidator() { return validator_; }

  void AddObject(uint32_t objnum,
                 RetainPtr<CPDF_Object> object,
                 ObjectState state) {
    ObjectData object_data;
    object_data.object = std::move(object);
    object_data.state = state;
    DCHECK(objects_data_.find(objnum) == objects_data_.end());
    objects_data_[objnum] = std::move(object_data);
  }

  void SetObjectState(uint32_t objnum, ObjectState state) {
    auto it = objects_data_.find(objnum);
    DCHECK(it != objects_data_.end());
    ObjectData& obj_data = it->second;
    obj_data.state = state;
  }

  CPDF_Object* GetTestObject(uint32_t objnum) {
    auto it = objects_data_.find(objnum);
    if (it == objects_data_.end()) {
      return nullptr;
    }
    return it->second.object.Get();
  }

 private:
  struct ObjectData {
    RetainPtr<CPDF_Object> object;
    ObjectState state = ObjectState::Unavailable;
  };
  std::map<uint32_t, ObjectData> objects_data_;
  RetainPtr<TestReadValidator> validator_;
};

}  // namespace

TEST(PageObjectAvailTest, ExcludePages) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(1)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "Kids", &holder, 2);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_Array>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(2)->AsMutableArray()->AppendNew<CPDF_Reference>(&holder,
                                                                       3);

  holder.AddObject(3, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(3)->GetMutableDict()->SetFor(
      "Type", pdfium::MakeRetain<CPDF_Name>(nullptr, "Page"));
  holder.GetTestObject(3)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "OtherPageData", &holder, 4);
  // Add unavailable object related to other page.
  holder.AddObject(4,
                   pdfium::MakeRetain<CPDF_String>(nullptr, "Other page data"),
                   TestHolder::ObjectState::Unavailable);

  CPDF_PageObjectAvail avail(holder.GetValidator(), &holder, 1);
  // Now object should be available, although the object '4' is not available,
  // because it is in skipped other page.
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}
