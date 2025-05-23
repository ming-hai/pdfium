// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/fx_coordinates.h"

#include <limits>
#include <vector>

#include "core/fxcrt/fx_coordinates_test_support.h"
#include "core/fxcrt/fx_system.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr float kMinFloat = std::numeric_limits<float>::min();
constexpr int kMaxInt = std::numeric_limits<int>::max();
constexpr int kMinInt = std::numeric_limits<int>::min();
constexpr float kMinIntAsFloat = static_cast<float>(kMinInt);
constexpr float kMaxIntAsFloat = static_cast<float>(kMaxInt);

}  // namespace

TEST(CFXFloatRectTest, FromFXRect) {
  FX_RECT downwards(10, 20, 30, 40);
  CFX_FloatRect rect(downwards);
  EXPECT_FLOAT_EQ(rect.left, 10.0f);
  EXPECT_FLOAT_EQ(rect.bottom, 20.0f);
  EXPECT_FLOAT_EQ(rect.right, 30.0f);
  EXPECT_FLOAT_EQ(rect.top, 40.0f);
}

TEST(CFXFloatRectTest, GetBBox) {
  CFX_FloatRect rect = CFX_FloatRect::GetBBox({});
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);

  std::vector<CFX_PointF> data;
  data.emplace_back(0.0f, 0.0f);
  rect = CFX_FloatRect::GetBBox(pdfium::span(data).first(0u));
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect = CFX_FloatRect::GetBBox(data);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);

  data.emplace_back(2.5f, 6.2f);
  data.emplace_back(1.5f, 6.2f);
  rect = CFX_FloatRect::GetBBox(pdfium::span(data).first(2u));
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(2.5f, rect.right);
  EXPECT_FLOAT_EQ(6.2f, rect.top);

  rect = CFX_FloatRect::GetBBox(data);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(2.5f, rect.right);
  EXPECT_FLOAT_EQ(6.2f, rect.top);

  data.emplace_back(2.5f, 6.3f);
  rect = CFX_FloatRect::GetBBox(data);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(2.5f, rect.right);
  EXPECT_FLOAT_EQ(6.3f, rect.top);

  data.emplace_back(-3.0f, 6.3f);
  rect = CFX_FloatRect::GetBBox(data);
  EXPECT_FLOAT_EQ(-3.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(2.5f, rect.right);
  EXPECT_FLOAT_EQ(6.3f, rect.top);

  data.emplace_back(4.0f, -8.0f);
  rect = CFX_FloatRect::GetBBox(data);
  EXPECT_FLOAT_EQ(-3.0f, rect.left);
  EXPECT_FLOAT_EQ(-8.0f, rect.bottom);
  EXPECT_FLOAT_EQ(4.0f, rect.right);
  EXPECT_FLOAT_EQ(6.3f, rect.top);
}

TEST(CFXFloatRectTest, GetInnerRect) {
  FX_RECT inner_rect;
  CFX_FloatRect rect;

  inner_rect = rect.GetInnerRect();
  EXPECT_EQ(0, inner_rect.left);
  EXPECT_EQ(0, inner_rect.bottom);
  EXPECT_EQ(0, inner_rect.right);
  EXPECT_EQ(0, inner_rect.top);

  // Function converts from float to int using floor() for top and right, and
  // ceil() for left and bottom.
  rect = CFX_FloatRect(-1.1f, 3.6f, 4.4f, -5.7f);
  inner_rect = rect.GetInnerRect();
  EXPECT_EQ(-1, inner_rect.left);
  EXPECT_EQ(4, inner_rect.bottom);
  EXPECT_EQ(4, inner_rect.right);
  EXPECT_EQ(-6, inner_rect.top);

  rect = CFX_FloatRect(kMinFloat, kMinFloat, kMinFloat, kMinFloat);
  inner_rect = rect.GetInnerRect();
  EXPECT_EQ(0, inner_rect.left);
  EXPECT_EQ(1, inner_rect.bottom);
  EXPECT_EQ(1, inner_rect.right);
  EXPECT_EQ(0, inner_rect.top);

  rect = CFX_FloatRect(-kMinFloat, -kMinFloat, -kMinFloat, -kMinFloat);
  inner_rect = rect.GetInnerRect();
  EXPECT_EQ(-1, inner_rect.left);
  EXPECT_EQ(0, inner_rect.bottom);
  EXPECT_EQ(0, inner_rect.right);
  EXPECT_EQ(-1, inner_rect.top);

  // Check at limits of integer range. When saturated would expect to get values
  // that are clamped to the limits of integers, but instead it is returning all
  // negative values that represent a rectangle as a dot in a far reach of the
  // negative coordinate space.  Related to crbug.com/1019026
  rect = CFX_FloatRect(kMinIntAsFloat, kMinIntAsFloat, kMaxIntAsFloat,
                       kMaxIntAsFloat);
  inner_rect = rect.GetInnerRect();
  EXPECT_EQ(kMinInt, inner_rect.left);
  EXPECT_EQ(kMaxInt, inner_rect.bottom);
  EXPECT_EQ(kMaxInt, inner_rect.right);
  EXPECT_EQ(kMinInt, inner_rect.top);

  rect = CFX_FloatRect(kMinIntAsFloat - 1.0f, kMinIntAsFloat - 1.0f,
                       kMaxIntAsFloat + 1.0f, kMaxIntAsFloat + 1.0f);
  inner_rect = rect.GetInnerRect();
  EXPECT_EQ(kMinInt, inner_rect.left);
  EXPECT_EQ(kMaxInt, inner_rect.bottom);
  EXPECT_EQ(kMaxInt, inner_rect.right);
  EXPECT_EQ(kMinInt, inner_rect.top);
}

TEST(CFXFloatRectTest, GetOuterRect) {
  FX_RECT outer_rect;
  CFX_FloatRect rect;

  outer_rect = rect.GetOuterRect();
  EXPECT_EQ(0, outer_rect.left);
  EXPECT_EQ(0, outer_rect.bottom);
  EXPECT_EQ(0, outer_rect.right);
  EXPECT_EQ(0, outer_rect.top);

  // Function converts from float to int using floor() for left and bottom, and
  // ceil() for right and top.
  rect = CFX_FloatRect(-1.1f, 3.6f, 4.4f, -5.7f);
  outer_rect = rect.GetOuterRect();
  EXPECT_EQ(-2, outer_rect.left);
  EXPECT_EQ(3, outer_rect.bottom);
  EXPECT_EQ(5, outer_rect.right);
  EXPECT_EQ(-5, outer_rect.top);

  rect = CFX_FloatRect(kMinFloat, kMinFloat, kMinFloat, kMinFloat);
  outer_rect = rect.GetOuterRect();
  EXPECT_EQ(0, outer_rect.left);
  EXPECT_EQ(1, outer_rect.bottom);
  EXPECT_EQ(1, outer_rect.right);
  EXPECT_EQ(0, outer_rect.top);

  rect = CFX_FloatRect(-kMinFloat, -kMinFloat, -kMinFloat, -kMinFloat);
  outer_rect = rect.GetOuterRect();
  EXPECT_EQ(-1, outer_rect.left);
  EXPECT_EQ(0, outer_rect.bottom);
  EXPECT_EQ(0, outer_rect.right);
  EXPECT_EQ(-1, outer_rect.top);

  // Check at limits of integer range. When saturated would expect to get values
  // that are clamped to the limits of integers.
  rect = CFX_FloatRect(kMinIntAsFloat, kMinIntAsFloat, kMaxIntAsFloat,
                       kMaxIntAsFloat);
  outer_rect = rect.GetOuterRect();
  EXPECT_EQ(kMinInt, outer_rect.left);
  EXPECT_EQ(kMaxInt, outer_rect.bottom);
  EXPECT_EQ(kMaxInt, outer_rect.right);
  EXPECT_EQ(kMinInt, outer_rect.top);

  rect = CFX_FloatRect(kMinIntAsFloat - 1.0f, kMinIntAsFloat - 1.0f,
                       kMaxIntAsFloat + 1.0f, kMaxIntAsFloat + 1.0f);
  outer_rect = rect.GetOuterRect();
  EXPECT_EQ(kMinInt, outer_rect.left);
  EXPECT_EQ(kMaxInt, outer_rect.bottom);
  EXPECT_EQ(kMaxInt, outer_rect.right);
  EXPECT_EQ(kMinInt, outer_rect.top);
}

TEST(CFXFloatRectTest, Normalize) {
  CFX_FloatRect rect;
  rect.Normalize();
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);

  rect = CFX_FloatRect(-1.0f, -3.0f, 4.5f, 3.2f);
  rect.Normalize();
  EXPECT_FLOAT_EQ(-1.0f, rect.left);
  EXPECT_FLOAT_EQ(-3.0f, rect.bottom);
  EXPECT_FLOAT_EQ(4.5f, rect.right);
  EXPECT_FLOAT_EQ(3.2f, rect.top);
  rect.Scale(-1.0f);
  rect.Normalize();
  EXPECT_FLOAT_EQ(-4.5f, rect.left);
  EXPECT_FLOAT_EQ(-3.2f, rect.bottom);
  EXPECT_FLOAT_EQ(1.0f, rect.right);
  EXPECT_FLOAT_EQ(3.0f, rect.top);
}

TEST(CFXFloatRectTest, Scale) {
  CFX_FloatRect rect(-1.0f, -3.0f, 4.5f, 3.2f);
  rect.Scale(1.0f);
  EXPECT_FLOAT_EQ(-1.0f, rect.left);
  EXPECT_FLOAT_EQ(-3.0f, rect.bottom);
  EXPECT_FLOAT_EQ(4.5f, rect.right);
  EXPECT_FLOAT_EQ(3.2f, rect.top);
  rect.Scale(0.5f);
  EXPECT_FLOAT_EQ(-0.5, rect.left);
  EXPECT_FLOAT_EQ(-1.5, rect.bottom);
  EXPECT_FLOAT_EQ(2.25f, rect.right);
  EXPECT_FLOAT_EQ(1.6f, rect.top);
  rect.Scale(2.0f);
  EXPECT_FLOAT_EQ(-1.0f, rect.left);
  EXPECT_FLOAT_EQ(-3.0f, rect.bottom);
  EXPECT_FLOAT_EQ(4.5f, rect.right);
  EXPECT_FLOAT_EQ(3.2f, rect.top);
  rect.Scale(-1.0f);
  EXPECT_FLOAT_EQ(1.0f, rect.left);
  EXPECT_FLOAT_EQ(3.0f, rect.bottom);
  EXPECT_FLOAT_EQ(-4.5f, rect.right);
  EXPECT_FLOAT_EQ(-3.2f, rect.top);
  rect.Scale(0.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
}

TEST(CFXFloatRectTest, ScaleEmpty) {
  CFX_FloatRect rect;
  rect.Scale(1.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect.Scale(0.5f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect.Scale(2.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect.Scale(0.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
}

TEST(CFXFloatRectTest, ScaleFromCenterPoint) {
  CFX_FloatRect rect(-1.0f, -3.0f, 4.5f, 3.2f);
  rect.ScaleFromCenterPoint(1.0f);
  EXPECT_FLOAT_EQ(-1.0f, rect.left);
  EXPECT_FLOAT_EQ(-3.0f, rect.bottom);
  EXPECT_FLOAT_EQ(4.5f, rect.right);
  EXPECT_FLOAT_EQ(3.2f, rect.top);
  rect.ScaleFromCenterPoint(0.5f);
  EXPECT_FLOAT_EQ(0.375f, rect.left);
  EXPECT_FLOAT_EQ(-1.45f, rect.bottom);
  EXPECT_FLOAT_EQ(3.125f, rect.right);
  EXPECT_FLOAT_EQ(1.65f, rect.top);
  rect.ScaleFromCenterPoint(2.0f);
  EXPECT_FLOAT_EQ(-1.0f, rect.left);
  EXPECT_FLOAT_EQ(-3.0f, rect.bottom);
  EXPECT_FLOAT_EQ(4.5f, rect.right);
  EXPECT_FLOAT_EQ(3.2f, rect.top);
  rect.ScaleFromCenterPoint(-1.0f);
  EXPECT_FLOAT_EQ(4.5f, rect.left);
  EXPECT_FLOAT_EQ(3.2f, rect.bottom);
  EXPECT_FLOAT_EQ(-1.0f, rect.right);
  EXPECT_FLOAT_EQ(-3.0f, rect.top);
  rect.ScaleFromCenterPoint(0.0f);
  EXPECT_FLOAT_EQ(1.75f, rect.left);
  EXPECT_NEAR(0.1f, rect.bottom, 0.001f);
  EXPECT_FLOAT_EQ(1.75f, rect.right);
  EXPECT_NEAR(0.1f, rect.top, 0.001f);
}

TEST(CFXFloatRectTest, ScaleFromCenterPointEmpty) {
  CFX_FloatRect rect;
  rect.ScaleFromCenterPoint(1.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect.ScaleFromCenterPoint(0.5f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect.ScaleFromCenterPoint(2.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
  rect.ScaleFromCenterPoint(0.0f);
  EXPECT_FLOAT_EQ(0.0f, rect.left);
  EXPECT_FLOAT_EQ(0.0f, rect.bottom);
  EXPECT_FLOAT_EQ(0.0f, rect.right);
  EXPECT_FLOAT_EQ(0.0f, rect.top);
}

TEST(CFXFloatRectTest, Print) {
  std::ostringstream os;
  CFX_FloatRect rect;
  os << rect;
  EXPECT_EQ("rect[w 0 x h 0 (left 0, bot 0)]", os.str());

  os.str("");
  rect = CFX_FloatRect(10, 20, 14, 23);
  os << rect;
  EXPECT_EQ("rect[w 4 x h 3 (left 10, bot 20)]", os.str());

  os.str("");
  rect = CFX_FloatRect(10.5, 20.5, 14.75, 23.75);
  os << rect;
  EXPECT_EQ("rect[w 4.25 x h 3.25 (left 10.5, bot 20.5)]", os.str());
}

TEST(CFXRectFTest, Print) {
  std::ostringstream os;
  CFX_RectF rect;
  os << rect;
  EXPECT_EQ("rect[w 0 x h 0 (left 0, top 0)]", os.str());

  os.str("");
  rect = CFX_RectF(10, 20, 4, 3);
  os << rect;
  EXPECT_EQ("rect[w 4 x h 3 (left 10, top 20)]", os.str());

  os.str("");
  rect = CFX_RectF(10.5, 20.5, 4.25, 3.25);
  os << rect;
  EXPECT_EQ("rect[w 4.25 x h 3.25 (left 10.5, top 20.5)]", os.str());
}

TEST(CFXMatrixTest, ReverseIdentity) {
  CFX_Matrix rev = CFX_Matrix().GetInverse();

  EXPECT_FLOAT_EQ(1.0, rev.a);
  EXPECT_FLOAT_EQ(0.0, rev.b);
  EXPECT_FLOAT_EQ(0.0, rev.c);
  EXPECT_FLOAT_EQ(1.0, rev.d);
  EXPECT_FLOAT_EQ(0.0, rev.e);
  EXPECT_FLOAT_EQ(0.0, rev.f);

  CFX_PointF expected(2, 3);
  CFX_PointF result = rev.Transform(CFX_Matrix().Transform(CFX_PointF(2, 3)));
  EXPECT_FLOAT_EQ(expected.x, result.x);
  EXPECT_FLOAT_EQ(expected.y, result.y);
}

TEST(CFXMatrixTest, SetIdentity) {
  CFX_Matrix m;
  EXPECT_FLOAT_EQ(1.0f, m.a);
  EXPECT_FLOAT_EQ(0.0f, m.b);
  EXPECT_FLOAT_EQ(0.0f, m.c);
  EXPECT_FLOAT_EQ(1.0f, m.d);
  EXPECT_FLOAT_EQ(0.0f, m.e);
  EXPECT_FLOAT_EQ(0.0f, m.f);
  EXPECT_TRUE(m.IsIdentity());

  m.a = -1;
  EXPECT_FALSE(m.IsIdentity());

  m = CFX_Matrix();
  EXPECT_FLOAT_EQ(1.0f, m.a);
  EXPECT_FLOAT_EQ(0.0f, m.b);
  EXPECT_FLOAT_EQ(0.0f, m.c);
  EXPECT_FLOAT_EQ(1.0f, m.d);
  EXPECT_FLOAT_EQ(0.0f, m.e);
  EXPECT_FLOAT_EQ(0.0f, m.f);
  EXPECT_TRUE(m.IsIdentity());
}

TEST(CFXMatrixTest, GetInverse) {
  static constexpr CFX_Matrix m(3, 0, 2, 3, 1, 4);
  CFX_Matrix rev = m.GetInverse();

  EXPECT_FLOAT_EQ(0.33333334f, rev.a);
  EXPECT_FLOAT_EQ(0.0f, rev.b);
  EXPECT_FLOAT_EQ(-0.22222222f, rev.c);
  EXPECT_FLOAT_EQ(0.33333334f, rev.d);
  EXPECT_FLOAT_EQ(0.55555556f, rev.e);
  EXPECT_FLOAT_EQ(-1.3333334f, rev.f);

  CFX_PointF expected(2, 3);
  CFX_PointF result = rev.Transform(m.Transform(CFX_PointF(2, 3)));
  EXPECT_FLOAT_EQ(expected.x, result.x);
  EXPECT_FLOAT_EQ(expected.y, result.y);
}

// Note, I think these are a bug and the matrix should be the identity.
TEST(CFXMatrixTest, GetInverseCR702041) {
  // The determinate is < std::numeric_limits<float>::epsilon()
  static constexpr CFX_Matrix m(0.947368443f, -0.108947366f, -0.923076928f,
                                0.106153846f, 18.0f, 787.929993f);
  CFX_Matrix rev = m.GetInverse();

  EXPECT_FLOAT_EQ(14247728.0f, rev.a);
  EXPECT_FLOAT_EQ(14622668.0f, rev.b);
  EXPECT_FLOAT_EQ(1.2389329e+08f, rev.c);
  EXPECT_FLOAT_EQ(1.2715364e+08f, rev.d);
  EXPECT_FLOAT_EQ(-9.7875698e+10f, rev.e);
  EXPECT_FLOAT_EQ(-1.0045138e+11f, rev.f);

  // Should be 2, 3
  CFX_PointF expected(0, 0);
  CFX_PointF result = rev.Transform(m.Transform(CFX_PointF(2, 3)));
  EXPECT_FLOAT_EQ(expected.x, result.x);
  EXPECT_FLOAT_EQ(expected.y, result.y);
}

TEST(CFXMatrixTest, GetInverseCR714187) {
  // The determinate is < std::numeric_limits<float>::epsilon()
  static constexpr CFX_Matrix m(0.000037f, 0.0f, 0.0f, -0.000037f, 182.413101f,
                                136.977646f);
  CFX_Matrix rev = m.GetInverse();

  EXPECT_FLOAT_EQ(27027.025f, rev.a);
  EXPECT_FLOAT_EQ(0.0f, rev.b);
  EXPECT_FLOAT_EQ(0.0f, rev.c);
  EXPECT_FLOAT_EQ(-27027.025f, rev.d);
  EXPECT_FLOAT_EQ(-4930083.5f, rev.e);
  EXPECT_FLOAT_EQ(3702098.2f, rev.f);

  // Should be 3 ....
  CFX_PointF expected(2, 2.75);
  CFX_PointF result = rev.Transform(m.Transform(CFX_PointF(2, 3)));
  EXPECT_FLOAT_EQ(expected.x, result.x);
  EXPECT_FLOAT_EQ(expected.y, result.y);
}

#define EXPECT_NEAR_FIVE_PLACES(a, b) EXPECT_NEAR((a), (b), 1e-5)

TEST(CFXMatrixTest, ComposeTransformations) {
  // sin(FXSYS_PI/2) and cos(FXSYS_PI/2) have a tiny error and are not
  // exactly 1.0f and 0.0f. The rotation matrix is thus not perfect.

  CFX_Matrix rotate_90;
  rotate_90.Rotate(FXSYS_PI / 2);
  EXPECT_NEAR_FIVE_PLACES(0.0f, rotate_90.a);
  EXPECT_NEAR_FIVE_PLACES(1.0f, rotate_90.b);
  EXPECT_NEAR_FIVE_PLACES(-1.0f, rotate_90.c);
  EXPECT_NEAR_FIVE_PLACES(0.0f, rotate_90.d);
  EXPECT_FLOAT_EQ(0.0f, rotate_90.e);
  EXPECT_FLOAT_EQ(0.0f, rotate_90.f);

  CFX_Matrix translate_23_11;
  translate_23_11.Translate(23, 11);
  EXPECT_FLOAT_EQ(1.0f, translate_23_11.a);
  EXPECT_FLOAT_EQ(0.0f, translate_23_11.b);
  EXPECT_FLOAT_EQ(0.0f, translate_23_11.c);
  EXPECT_FLOAT_EQ(1.0f, translate_23_11.d);
  EXPECT_FLOAT_EQ(23.0f, translate_23_11.e);
  EXPECT_FLOAT_EQ(11.0f, translate_23_11.f);

  CFX_Matrix scale_5_13;
  scale_5_13.Scale(5, 13);
  EXPECT_FLOAT_EQ(5.0f, scale_5_13.a);
  EXPECT_FLOAT_EQ(0.0f, scale_5_13.b);
  EXPECT_FLOAT_EQ(0.0f, scale_5_13.c);
  EXPECT_FLOAT_EQ(13.0f, scale_5_13.d);
  EXPECT_FLOAT_EQ(0.0, scale_5_13.e);
  EXPECT_FLOAT_EQ(0.0, scale_5_13.f);

  // Apply the transforms to points step by step.
  CFX_PointF origin_transformed(0, 0);
  CFX_PointF p_10_20_transformed(10, 20);

  origin_transformed = rotate_90.Transform(origin_transformed);
  EXPECT_FLOAT_EQ(0.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(0.0f, origin_transformed.y);
  p_10_20_transformed = rotate_90.Transform(p_10_20_transformed);
  EXPECT_FLOAT_EQ(-20.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(10.0f, p_10_20_transformed.y);

  origin_transformed = translate_23_11.Transform(origin_transformed);
  EXPECT_FLOAT_EQ(23.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(11.0f, origin_transformed.y);
  p_10_20_transformed = translate_23_11.Transform(p_10_20_transformed);
  EXPECT_FLOAT_EQ(3.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(21.0f, p_10_20_transformed.y);

  origin_transformed = scale_5_13.Transform(origin_transformed);
  EXPECT_FLOAT_EQ(115.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(143.0f, origin_transformed.y);
  p_10_20_transformed = scale_5_13.Transform(p_10_20_transformed);
  EXPECT_FLOAT_EQ(15.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(273.0f, p_10_20_transformed.y);

  // Apply the transforms to points in the reverse order.
  origin_transformed = CFX_PointF(0, 0);
  p_10_20_transformed = CFX_PointF(10, 20);

  origin_transformed = scale_5_13.Transform(origin_transformed);
  EXPECT_FLOAT_EQ(0.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(0.0f, origin_transformed.y);
  p_10_20_transformed = scale_5_13.Transform(p_10_20_transformed);
  EXPECT_FLOAT_EQ(50.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(260.0f, p_10_20_transformed.y);

  origin_transformed = translate_23_11.Transform(origin_transformed);
  EXPECT_FLOAT_EQ(23.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(11.0f, origin_transformed.y);
  p_10_20_transformed = translate_23_11.Transform(p_10_20_transformed);
  EXPECT_FLOAT_EQ(73.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(271.0f, p_10_20_transformed.y);

  origin_transformed = rotate_90.Transform(origin_transformed);
  EXPECT_FLOAT_EQ(-11.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(23.0f, origin_transformed.y);
  p_10_20_transformed = rotate_90.Transform(p_10_20_transformed);
  EXPECT_FLOAT_EQ(-271.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(73.0f, p_10_20_transformed.y);

  // Compose all transforms.
  CFX_Matrix m;
  m.Concat(rotate_90);
  m.Concat(translate_23_11);
  m.Concat(scale_5_13);
  EXPECT_NEAR_FIVE_PLACES(0.0f, m.a);
  EXPECT_NEAR_FIVE_PLACES(13.0f, m.b);
  EXPECT_NEAR_FIVE_PLACES(-5.0f, m.c);
  EXPECT_NEAR_FIVE_PLACES(0.0f, m.d);
  EXPECT_FLOAT_EQ(115.0, m.e);
  EXPECT_FLOAT_EQ(143.0, m.f);

  // Note how the results using the combined matrix are equal to the results
  // when applying the three original matrices step-by-step.
  origin_transformed = m.Transform(CFX_PointF(0, 0));
  EXPECT_FLOAT_EQ(115.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(143.0f, origin_transformed.y);

  p_10_20_transformed = m.Transform(CFX_PointF(10, 20));
  EXPECT_FLOAT_EQ(15.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(273.0f, p_10_20_transformed.y);

  // Now compose all transforms prepending.
  m = CFX_Matrix();
  m = rotate_90 * m;
  m = translate_23_11 * m;
  m = scale_5_13 * m;
  EXPECT_NEAR_FIVE_PLACES(0.0f, m.a);
  EXPECT_NEAR_FIVE_PLACES(5.0f, m.b);
  EXPECT_NEAR_FIVE_PLACES(-13.0f, m.c);
  EXPECT_NEAR_FIVE_PLACES(0.0f, m.d);
  EXPECT_FLOAT_EQ(-11.0, m.e);
  EXPECT_FLOAT_EQ(23.0, m.f);

  // Note how the results using the combined matrix are equal to the results
  // when applying the three original matrices step-by-step in the reverse
  // order.
  origin_transformed = m.Transform(CFX_PointF(0, 0));
  EXPECT_FLOAT_EQ(-11.0f, origin_transformed.x);
  EXPECT_FLOAT_EQ(23.0f, origin_transformed.y);

  p_10_20_transformed = m.Transform(CFX_PointF(10, 20));
  EXPECT_FLOAT_EQ(-271.0f, p_10_20_transformed.x);
  EXPECT_FLOAT_EQ(73.0f, p_10_20_transformed.y);
}

TEST(CFXMatrixTest, TransformRectForRectF) {
  CFX_Matrix rotate_90;
  rotate_90.Rotate(FXSYS_PI / 2);

  CFX_Matrix scale_5_13;
  scale_5_13.Scale(5, 13);

  CFX_RectF rect(10.5f, 20.5f, 4.25f, 3.25f);
  rect = rotate_90.TransformRect(rect);
  EXPECT_FLOAT_EQ(-23.75f, rect.Left());
  EXPECT_FLOAT_EQ(10.5f, rect.Top());
  EXPECT_FLOAT_EQ(3.25f, rect.Width());
  EXPECT_FLOAT_EQ(4.25f, rect.Height());

  rect = scale_5_13.TransformRect(rect);
  EXPECT_FLOAT_EQ(-118.75f, rect.Left());
  EXPECT_FLOAT_EQ(136.5f, rect.Top());
  EXPECT_FLOAT_EQ(16.25f, rect.Width());
  EXPECT_FLOAT_EQ(55.25f, rect.Height());
}

TEST(CFXMatrixTest, TransformRectForFloatRect) {
  CFX_Matrix rotate_90;
  rotate_90.Rotate(FXSYS_PI / 2);

  CFX_Matrix scale_5_13;
  scale_5_13.Scale(5, 13);

  CFX_FloatRect rect(5.5f, 0.0f, 12.25f, 2.7f);
  rect = rotate_90.TransformRect(rect);
  EXPECT_FLOAT_EQ(-2.7f, rect.Left());
  EXPECT_FLOAT_EQ(5.5f, rect.Bottom());
  EXPECT_NEAR(0.0f, rect.Right(), 0.00001f);
  EXPECT_FLOAT_EQ(12.25f, rect.Top());

  rect = scale_5_13.TransformRect(rect);
  EXPECT_FLOAT_EQ(-13.5f, rect.Left());
  EXPECT_FLOAT_EQ(71.5f, rect.Bottom());
  EXPECT_NEAR(0.0f, rect.Right(), 0.00001f);
  EXPECT_FLOAT_EQ(159.25f, rect.Top());
}
