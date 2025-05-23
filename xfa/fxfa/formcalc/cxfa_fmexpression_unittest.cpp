// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xfa/fxfa/formcalc/cxfa_fmexpression.h"

#include <utility>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/widetext_buffer.h"
#include "testing/fxgc_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/cppgc/heap.h"
#include "xfa/fxfa/formcalc/cxfa_fmlexer.h"
#include "xfa/fxfa/formcalc/cxfa_fmtojavascriptdepth.h"

class FMExpressionTest : public FXGCUnitTest {};
class FMCallExpressionTest : public FXGCUnitTest {};
class FMStringExpressionTest : public FXGCUnitTest {};

TEST_F(FMCallExpressionTest, MoreThan32Arguments) {
  // Use sign as it has 3 object parameters at positions 0, 5, and 6.
  auto* exp = cppgc::MakeGarbageCollected<CXFA_FMIdentifierExpression>(
      heap()->GetAllocationHandle(), L"sign");

  std::vector<cppgc::Member<CXFA_FMSimpleExpression>> args;
  for (size_t i = 0; i < 50; i++) {
    args.push_back(cppgc::MakeGarbageCollected<CXFA_FMNullExpression>(
        heap()->GetAllocationHandle()));
  }
  CXFA_FMToJavaScriptDepth::Reset();
  auto* callExp = cppgc::MakeGarbageCollected<CXFA_FMCallExpression>(
      heap()->GetAllocationHandle(), exp, std::move(args), true);

  WideTextBuffer js;
  EXPECT_TRUE(callExp->ToJavaScript(
      &js, CXFA_FMAssignExpression::ReturnType::kInferred));

  // Generate the result javascript string.
  WideString result = L"sign(";
  for (size_t i = 0; i < 50; i++) {
    if (i > 0) {
      result += L", ";
    }

    result += L"pfm_rt.get_";
    // Object positions for sign() method.
    if (i == 0 || i == 5 || i == 6) {
      result += L"jsobj(null)";
    } else {
      result += L"val(null)";
    }
  }
  result += L")";

  EXPECT_EQ(result.AsStringView(), js.AsStringView());
}

TEST_F(FMStringExpressionTest, Empty) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;
  auto* exp = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), L"");
  EXPECT_TRUE(exp->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(L"", accumulator.AsStringView());
}

TEST_F(FMStringExpressionTest, Short) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;
  auto* exp = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), L"a");
  EXPECT_TRUE(exp->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(L"a", accumulator.AsStringView());
}

TEST_F(FMStringExpressionTest, Medium) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;
  auto* exp = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), L".abcd.");
  EXPECT_TRUE(exp->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(L"\"abcd\"", accumulator.AsStringView());
}

TEST_F(FMStringExpressionTest, Long) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;
  std::vector<WideStringView::UnsignedType> vec(140000, L'A');
  auto* exp = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), WideString(WideStringView(vec)));
  EXPECT_TRUE(exp->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(140000u, accumulator.GetLength());
}

TEST_F(FMStringExpressionTest, Quoted) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;
  auto* exp = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), L".Simon says \"\"run\"\".");
  EXPECT_TRUE(exp->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(L"\"Simon says \\\"run\\\"\"", accumulator.AsStringView());
}

TEST_F(FMExpressionTest, VarExpressionInitNull) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;

  auto* expr = cppgc::MakeGarbageCollected<CXFA_FMVarExpression>(
      heap()->GetAllocationHandle(), L"s", nullptr);
  EXPECT_TRUE(expr->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(
      LR"***(var s = "";
)***",
      accumulator.MakeString());
}

TEST_F(FMExpressionTest, VarExpressionInitBlank) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;

  auto* init = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), LR"("")");
  auto* expr = cppgc::MakeGarbageCollected<CXFA_FMVarExpression>(
      heap()->GetAllocationHandle(), L"s", init);
  EXPECT_TRUE(expr->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(
      LR"***(var s = "";
s = pfm_rt.var_filter(s);
)***",
      accumulator.MakeString());
}

TEST_F(FMExpressionTest, VarExpressionInitString) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;

  auto* init = cppgc::MakeGarbageCollected<CXFA_FMStringExpression>(
      heap()->GetAllocationHandle(), LR"("foo")");
  auto* expr = cppgc::MakeGarbageCollected<CXFA_FMVarExpression>(
      heap()->GetAllocationHandle(), L"s", init);
  EXPECT_TRUE(expr->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(
      LR"***(var s = "foo";
s = pfm_rt.var_filter(s);
)***",
      accumulator.MakeString());
}

TEST_F(FMExpressionTest, VarExpressionInitNumeric) {
  CXFA_FMToJavaScriptDepth::Reset();
  WideTextBuffer accumulator;

  auto* init = cppgc::MakeGarbageCollected<CXFA_FMNumberExpression>(
      heap()->GetAllocationHandle(), L"112");
  auto* expr = cppgc::MakeGarbageCollected<CXFA_FMVarExpression>(
      heap()->GetAllocationHandle(), L"s", init);
  EXPECT_TRUE(expr->ToJavaScript(
      &accumulator, CXFA_FMAssignExpression::ReturnType::kInferred));
  EXPECT_EQ(
      LR"***(var s = 112;
s = pfm_rt.var_filter(s);
)***",
      accumulator.MakeString());
}
