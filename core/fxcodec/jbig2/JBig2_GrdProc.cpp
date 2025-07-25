// Copyright 2015 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jbig2/JBig2_GrdProc.h"

#include <array>
#include <functional>
#include <memory>
#include <utility>

#include "core/fxcodec/fax/faxmodule.h"
#include "core/fxcodec/jbig2/JBig2_ArithDecoder.h"
#include "core/fxcodec/jbig2/JBig2_BitStream.h"
#include "core/fxcodec/jbig2/JBig2_Image.h"
#include "core/fxcrt/pauseindicator_iface.h"

namespace {

// TODO(npm): Name this constants better or merge some together.
constexpr std::array<const uint16_t, 3> kOptConstant1 = {
    {0x9b25, 0x0795, 0x00e5}};
constexpr std::array<const uint16_t, 3> kOptConstant2 = {
    {0x0006, 0x0004, 0x0001}};
constexpr std::array<const uint16_t, 3> kOptConstant3 = {
    {0xf800, 0x1e00, 0x0380}};
constexpr std::array<const uint16_t, 3> kOptConstant4 = {
    {0x0000, 0x0001, 0x0003}};
constexpr std::array<const uint16_t, 3> kOptConstant5 = {
    {0x07f0, 0x01f8, 0x007c}};
constexpr std::array<const uint16_t, 3> kOptConstant6 = {
    {0x7bf7, 0x0efb, 0x01bd}};
constexpr std::array<const uint16_t, 3> kOptConstant7 = {
    {0x0800, 0x0200, 0x0080}};
constexpr std::array<const uint16_t, 3> kOptConstant8 = {
    {0x0010, 0x0008, 0x0004}};
constexpr std::array<const uint16_t, 3> kOptConstant9 = {
    {0x000c, 0x0009, 0x0007}};
constexpr std::array<const uint16_t, 3> kOptConstant10 = {
    {0x0007, 0x000f, 0x0007}};
constexpr std::array<const uint16_t, 3> kOptConstant11 = {
    {0x001f, 0x001f, 0x000f}};
constexpr std::array<const uint16_t, 3> kOptConstant12 = {
    {0x000f, 0x0007, 0x0003}};

}  // namespace

CJBig2_GRDProc::ProgressiveArithDecodeState::ProgressiveArithDecodeState() =
    default;

CJBig2_GRDProc::ProgressiveArithDecodeState::~ProgressiveArithDecodeState() =
    default;

CJBig2_GRDProc::CJBig2_GRDProc() = default;

CJBig2_GRDProc::~CJBig2_GRDProc() = default;

bool CJBig2_GRDProc::UseTemplate0Opt3() const {
  return (GBAT[0] == 3) && (GBAT[1] == -1) && (GBAT[2] == -3) &&
         (GBAT[3] == -1) && (GBAT[4] == 2) && (GBAT[5] == -2) &&
         (GBAT[6] == -2) && (GBAT[7] == -2) && !USESKIP;
}

bool CJBig2_GRDProc::UseTemplate1Opt3() const {
  return (GBAT[0] == 3) && (GBAT[1] == -1) && !USESKIP;
}

bool CJBig2_GRDProc::UseTemplate23Opt3() const {
  return (GBAT[0] == 2) && (GBAT[1] == -1) && !USESKIP;
}

std::unique_ptr<CJBig2_Image> CJBig2_GRDProc::DecodeArith(
    CJBig2_ArithDecoder* pArithDecoder,
    pdfium::span<JBig2ArithCtx> gbContexts) {
  if (!CJBig2_Image::IsValidImageSize(GBW, GBH)) {
    return std::make_unique<CJBig2_Image>(GBW, GBH);
  }

  switch (GBTEMPLATE) {
    case 0:
      return UseTemplate0Opt3()
                 ? DecodeArithOpt3(pArithDecoder, gbContexts, 0)
                 : DecodeArithTemplateUnopt(pArithDecoder, gbContexts, 0);
    case 1:
      return UseTemplate1Opt3()
                 ? DecodeArithOpt3(pArithDecoder, gbContexts, 1)
                 : DecodeArithTemplateUnopt(pArithDecoder, gbContexts, 1);
    case 2:
      return UseTemplate23Opt3()
                 ? DecodeArithOpt3(pArithDecoder, gbContexts, 2)
                 : DecodeArithTemplateUnopt(pArithDecoder, gbContexts, 2);
    default:
      return UseTemplate23Opt3()
                 ? DecodeArithTemplate3Opt3(pArithDecoder, gbContexts)
                 : DecodeArithTemplate3Unopt(pArithDecoder, gbContexts);
  }
}

std::unique_ptr<CJBig2_Image> CJBig2_GRDProc::DecodeArithOpt3(
    CJBig2_ArithDecoder* pArithDecoder,
    pdfium::span<JBig2ArithCtx> gbContexts,
    int OPT) {
  auto GBREG = std::make_unique<CJBig2_Image>(GBW, GBH);
  if (!GBREG->data()) {
    return nullptr;
  }

  int LTP = 0;
  uint8_t* pLine = GBREG->data();
  int32_t nStride = GBREG->stride();
  int32_t nStride2 = nStride << 1;
  int32_t nLineBytes = ((GBW + 7) >> 3) - 1;
  int32_t nBitsLeft = GBW - (nLineBytes << 3);
  // TODO(npm): Why is the height only trimmed when OPT is 0?
  uint32_t height = OPT == 0 ? GBH & 0x7fffffff : GBH;
  UNSAFE_TODO({
    for (uint32_t h = 0; h < height; ++h) {
      if (TPGDON) {
        if (pArithDecoder->IsComplete()) {
          return nullptr;
        }

        LTP = LTP ^ pArithDecoder->Decode(&gbContexts[kOptConstant1[OPT]]);
      }
      if (LTP) {
        GBREG->CopyLine(h, h - 1);
      } else {
        if (h > 1) {
          uint8_t* pLine1 = pLine - nStride2;
          uint8_t* pLine2 = pLine - nStride;
          uint32_t line1 = (*pLine1++) << kOptConstant2[OPT];
          uint32_t line2 = *pLine2++;
          uint32_t CONTEXT =
              (line1 & kOptConstant3[OPT]) |
              ((line2 >> kOptConstant4[OPT]) & kOptConstant5[OPT]);
          for (int32_t cc = 0; cc < nLineBytes; ++cc) {
            line1 = (line1 << 8) | ((*pLine1++) << kOptConstant2[OPT]);
            line2 = (line2 << 8) | (*pLine2++);
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; --k) {
              if (pArithDecoder->IsComplete()) {
                return nullptr;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT =
                  (((CONTEXT & kOptConstant6[OPT]) << 1) | bVal |
                   ((line1 >> k) & kOptConstant7[OPT]) |
                   ((line2 >> (k + kOptConstant4[OPT])) & kOptConstant8[OPT]));
            }
            pLine[cc] = cVal;
          }
          line1 <<= 8;
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; ++k) {
            if (pArithDecoder->IsComplete()) {
              return nullptr;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = (((CONTEXT & kOptConstant6[OPT]) << 1) | bVal |
                       ((line1 >> (7 - k)) & kOptConstant7[OPT]) |
                       ((line2 >> (7 + kOptConstant4[OPT] - k)) &
                        kOptConstant8[OPT]));
          }
          pLine[nLineBytes] = cVal1;
        } else {
          uint8_t* pLine2 = pLine - nStride;
          uint32_t line2 = (h & 1) ? (*pLine2++) : 0;
          uint32_t CONTEXT =
              ((line2 >> kOptConstant4[OPT]) & kOptConstant5[OPT]);
          for (int32_t cc = 0; cc < nLineBytes; ++cc) {
            if (h & 1) {
              line2 = (line2 << 8) | (*pLine2++);
            }
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; --k) {
              if (pArithDecoder->IsComplete()) {
                return nullptr;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT =
                  (((CONTEXT & kOptConstant6[OPT]) << 1) | bVal |
                   ((line2 >> (k + kOptConstant4[OPT])) & kOptConstant8[OPT]));
            }
            pLine[cc] = cVal;
          }
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; ++k) {
            if (pArithDecoder->IsComplete()) {
              return nullptr;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = (((CONTEXT & kOptConstant6[OPT]) << 1) | bVal |
                       (((line2 >> (7 + kOptConstant4[OPT] - k))) &
                        kOptConstant8[OPT]));
          }
          pLine[nLineBytes] = cVal1;
        }
      }
      pLine += nStride;
    }
    return GBREG;
  });
}

std::unique_ptr<CJBig2_Image> CJBig2_GRDProc::DecodeArithTemplateUnopt(
    CJBig2_ArithDecoder* pArithDecoder,
    pdfium::span<JBig2ArithCtx> gbContexts,
    int UNOPT) {
  auto GBREG = std::make_unique<CJBig2_Image>(GBW, GBH);
  if (!GBREG->data()) {
    return nullptr;
  }

  GBREG->Fill(false);
  int LTP = 0;
  uint8_t MOD2 = UNOPT % 2;
  uint8_t DIV2 = UNOPT / 2;
  uint8_t SHIFT = 4 - UNOPT;
  for (uint32_t h = 0; h < GBH; h++) {
    if (TPGDON) {
      if (pArithDecoder->IsComplete()) {
        return nullptr;
      }

      LTP = LTP ^ pArithDecoder->Decode(&gbContexts[kOptConstant1[UNOPT]]);
    }
    if (LTP) {
      GBREG->CopyLine(h, h - 1);
      continue;
    }
    uint32_t line1 = GBREG->GetPixel(1 + MOD2, h - 2);
    line1 |= GBREG->GetPixel(MOD2, h - 2) << 1;
    if (UNOPT == 1) {
      line1 |= GBREG->GetPixel(0, h - 2) << 2;
    }
    uint32_t line2 = GBREG->GetPixel(2 - DIV2, h - 1);
    line2 |= GBREG->GetPixel(1 - DIV2, h - 1) << 1;
    if (UNOPT < 2) {
      line2 |= GBREG->GetPixel(0, h - 1) << 2;
    }
    uint32_t line3 = 0;
    for (uint32_t w = 0; w < GBW; w++) {
      int bVal = 0;
      if (!USESKIP || !SKIP->GetPixel(w, h)) {
        if (pArithDecoder->IsComplete()) {
          return nullptr;
        }

        uint32_t CONTEXT = line3;
        CONTEXT |= GBREG->GetPixel(w + GBAT[0], h + GBAT[1]) << SHIFT;
        CONTEXT |= line2 << (SHIFT + 1);
        CONTEXT |= line1 << kOptConstant9[UNOPT];
        if (UNOPT == 0) {
          CONTEXT |= GBREG->GetPixel(w + GBAT[2], h + GBAT[3]) << 10;
          CONTEXT |= GBREG->GetPixel(w + GBAT[4], h + GBAT[5]) << 11;
          CONTEXT |= GBREG->GetPixel(w + GBAT[6], h + GBAT[7]) << 15;
        }
        bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
        if (bVal) {
          GBREG->SetPixel(w, h, bVal);
        }
      }
      line1 = ((line1 << 1) | GBREG->GetPixel(w + 2 + MOD2, h - 2)) &
              kOptConstant10[UNOPT];
      line2 = ((line2 << 1) | GBREG->GetPixel(w + 3 - DIV2, h - 1)) &
              kOptConstant11[UNOPT];
      line3 = ((line3 << 1) | bVal) & kOptConstant12[UNOPT];
    }
  }
  return GBREG;
}

std::unique_ptr<CJBig2_Image> CJBig2_GRDProc::DecodeArithTemplate3Opt3(
    CJBig2_ArithDecoder* pArithDecoder,
    pdfium::span<JBig2ArithCtx> gbContexts) {
  auto GBREG = std::make_unique<CJBig2_Image>(GBW, GBH);
  if (!GBREG->data()) {
    return nullptr;
  }

  int LTP = 0;
  uint8_t* pLine = GBREG->data();
  int32_t nStride = GBREG->stride();
  int32_t nLineBytes = ((GBW + 7) >> 3) - 1;
  int32_t nBitsLeft = GBW - (nLineBytes << 3);

  UNSAFE_TODO({
    for (uint32_t h = 0; h < GBH; h++) {
      if (TPGDON) {
        if (pArithDecoder->IsComplete()) {
          return nullptr;
        }

        LTP = LTP ^ pArithDecoder->Decode(&gbContexts[0x0195]);
      }

      if (LTP) {
        GBREG->CopyLine(h, h - 1);
      } else {
        if (h > 0) {
          uint8_t* pLine1 = pLine - nStride;
          uint32_t line1 = *pLine1++;
          uint32_t CONTEXT = (line1 >> 1) & 0x03f0;
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            line1 = (line1 << 8) | (*pLine1++);
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return nullptr;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal |
                        ((line1 >> (k + 1)) & 0x0010);
            }
            pLine[cc] = cVal;
          }
          line1 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return nullptr;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal |
                      ((line1 >> (8 - k)) & 0x0010);
          }
          pLine[nLineBytes] = cVal1;
        } else {
          uint32_t CONTEXT = 0;
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return nullptr;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal;
            }
            pLine[cc] = cVal;
          }
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return nullptr;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal;
          }
          pLine[nLineBytes] = cVal1;
        }
      }
      pLine += nStride;
    }
    return GBREG;
  });
}

std::unique_ptr<CJBig2_Image> CJBig2_GRDProc::DecodeArithTemplate3Unopt(
    CJBig2_ArithDecoder* pArithDecoder,
    pdfium::span<JBig2ArithCtx> gbContexts) {
  auto GBREG = std::make_unique<CJBig2_Image>(GBW, GBH);
  if (!GBREG->data()) {
    return nullptr;
  }

  GBREG->Fill(false);
  int LTP = 0;
  for (uint32_t h = 0; h < GBH; h++) {
    if (TPGDON) {
      if (pArithDecoder->IsComplete()) {
        return nullptr;
      }

      LTP = LTP ^ pArithDecoder->Decode(&gbContexts[0x0195]);
    }
    if (LTP == 1) {
      GBREG->CopyLine(h, h - 1);
    } else {
      uint32_t line1 = GBREG->GetPixel(1, h - 1);
      line1 |= GBREG->GetPixel(0, h - 1) << 1;
      uint32_t line2 = 0;
      for (uint32_t w = 0; w < GBW; w++) {
        int bVal;
        if (USESKIP && SKIP->GetPixel(w, h)) {
          bVal = 0;
        } else {
          uint32_t CONTEXT = line2;
          CONTEXT |= GBREG->GetPixel(w + GBAT[0], h + GBAT[1]) << 4;
          CONTEXT |= line1 << 5;
          if (pArithDecoder->IsComplete()) {
            return nullptr;
          }

          bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
        }
        if (bVal) {
          GBREG->SetPixel(w, h, bVal);
        }
        line1 = ((line1 << 1) | GBREG->GetPixel(w + 2, h - 1)) & 0x1f;
        line2 = ((line2 << 1) | bVal) & 0x0f;
      }
    }
  }
  return GBREG;
}

FXCODEC_STATUS CJBig2_GRDProc::StartDecodeArith(
    ProgressiveArithDecodeState* pState) {
  if (!CJBig2_Image::IsValidImageSize(GBW, GBH)) {
    progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
    return FXCODEC_STATUS::kDecodeFinished;
  }
  progressive_status_ = FXCODEC_STATUS::kDecodeReady;
  std::unique_ptr<CJBig2_Image>* pImage = pState->pImage;
  if (!*pImage) {
    *pImage = std::make_unique<CJBig2_Image>(GBW, GBH);
  }
  if (!(*pImage)->data()) {
    *pImage = nullptr;
    progressive_status_ = FXCODEC_STATUS::kError;
    return FXCODEC_STATUS::kError;
  }
  pImage->get()->Fill(false);
  decode_type_ = 1;
  ltp_ = 0;
  line_ = nullptr;
  loop_index_ = 0;
  return ProgressiveDecodeArith(pState);
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArith(
    ProgressiveArithDecodeState* pState) {
  int iline = loop_index_;

  using DecodeFunction = std::function<FXCODEC_STATUS(
      CJBig2_GRDProc&, ProgressiveArithDecodeState*)>;
  DecodeFunction func;
  switch (GBTEMPLATE) {
    case 0:
      func = UseTemplate0Opt3()
                 ? &CJBig2_GRDProc::ProgressiveDecodeArithTemplate0Opt3
                 : &CJBig2_GRDProc::ProgressiveDecodeArithTemplate0Unopt;
      break;
    case 1:
      func = UseTemplate1Opt3()
                 ? &CJBig2_GRDProc::ProgressiveDecodeArithTemplate1Opt3
                 : &CJBig2_GRDProc::ProgressiveDecodeArithTemplate1Unopt;
      break;
    case 2:
      func = UseTemplate23Opt3()
                 ? &CJBig2_GRDProc::ProgressiveDecodeArithTemplate2Opt3
                 : &CJBig2_GRDProc::ProgressiveDecodeArithTemplate2Unopt;
      break;
    default:
      func = UseTemplate23Opt3()
                 ? &CJBig2_GRDProc::ProgressiveDecodeArithTemplate3Opt3
                 : &CJBig2_GRDProc::ProgressiveDecodeArithTemplate3Unopt;
      break;
  }
  CJBig2_Image* pImage = pState->pImage->get();
  progressive_status_ = func(*this, pState);
  replace_rect_.left = 0;
  replace_rect_.right = pImage->width();
  replace_rect_.top = iline;
  replace_rect_.bottom = loop_index_;
  if (progressive_status_ == FXCODEC_STATUS::kDecodeFinished) {
    loop_index_ = 0;
  }

  return progressive_status_;
}

FXCODEC_STATUS CJBig2_GRDProc::StartDecodeMMR(
    std::unique_ptr<CJBig2_Image>* pImage,
    CJBig2_BitStream* pStream) {
  auto image = std::make_unique<CJBig2_Image>(GBW, GBH);
  if (!image->data()) {
    *pImage = nullptr;
    progressive_status_ = FXCODEC_STATUS::kError;
    return progressive_status_;
  }
  int bitpos = static_cast<int>(pStream->getBitPos());
  bitpos = FaxModule::FaxG4Decode(pStream->getBufSpan(), bitpos, GBW, GBH,
                                  image->stride(), image->data());
  pStream->setBitPos(bitpos);
  for (uint32_t i = 0; i < image->stride() * GBH; ++i) {
    UNSAFE_TODO(image->data()[i] = ~image->data()[i]);
  }
  progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
  *pImage = std::move(image);
  return progressive_status_;
}

FXCODEC_STATUS CJBig2_GRDProc::ContinueDecode(
    ProgressiveArithDecodeState* pState) {
  if (progressive_status_ != FXCODEC_STATUS::kDecodeToBeContinued) {
    return progressive_status_;
  }

  if (decode_type_ != 1) {
    progressive_status_ = FXCODEC_STATUS::kError;
    return progressive_status_;
  }
  return ProgressiveDecodeArith(pState);
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate0Opt3(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  if (!line_) {
    line_ = pImage->data();
  }
  int32_t nStride = pImage->stride();
  int32_t nStride2 = nStride << 1;
  int32_t nLineBytes = ((GBW + 7) >> 3) - 1;
  int32_t nBitsLeft = GBW - (nLineBytes << 3);
  uint32_t height = GBH & 0x7fffffff;

  UNSAFE_TODO({
    for (; loop_index_ < height; loop_index_++) {
      if (TPGDON) {
        if (pArithDecoder->IsComplete()) {
          return FXCODEC_STATUS::kError;
        }

        ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x9b25]);
      }
      if (ltp_) {
        pImage->CopyLine(loop_index_, loop_index_ - 1);
      } else {
        if (loop_index_ > 1) {
          uint8_t* pLine1 = line_ - nStride2;
          uint8_t* pLine2 = line_ - nStride;
          uint32_t line1 = (*pLine1++) << 6;
          uint32_t line2 = *pLine2++;
          uint32_t CONTEXT = ((line1 & 0xf800) | (line2 & 0x07f0));
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            line1 = (line1 << 8) | ((*pLine1++) << 6);
            line2 = (line2 << 8) | (*pLine2++);
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = (((CONTEXT & 0x7bf7) << 1) | bVal |
                         ((line1 >> k) & 0x0800) | ((line2 >> k) & 0x0010));
            }
            line_[cc] = cVal;
          }
          line1 <<= 8;
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT =
                (((CONTEXT & 0x7bf7) << 1) | bVal |
                 ((line1 >> (7 - k)) & 0x0800) | ((line2 >> (7 - k)) & 0x0010));
          }
          line_[nLineBytes] = cVal1;
        } else {
          uint8_t* pLine2 = line_ - nStride;
          uint32_t line2 = (loop_index_ & 1) ? (*pLine2++) : 0;
          uint32_t CONTEXT = (line2 & 0x07f0);
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            if (loop_index_ & 1) {
              line2 = (line2 << 8) | (*pLine2++);
            }
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT =
                  (((CONTEXT & 0x7bf7) << 1) | bVal | ((line2 >> k) & 0x0010));
            }
            line_[cc] = cVal;
          }
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = (((CONTEXT & 0x7bf7) << 1) | bVal |
                       ((line2 >> (7 - k)) & 0x0010));
          }
          line_[nLineBytes] = cVal1;
        }
      }
      line_ += nStride;
      if (pState->pPause && pState->pPause->NeedToPauseNow()) {
        loop_index_++;
        progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
        return FXCODEC_STATUS::kDecodeToBeContinued;
      }
    }
    progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
    return FXCODEC_STATUS::kDecodeFinished;
  });
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate0Unopt(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  for (; loop_index_ < GBH; loop_index_++) {
    if (TPGDON) {
      if (pArithDecoder->IsComplete()) {
        return FXCODEC_STATUS::kError;
      }

      ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x9b25]);
    }
    if (ltp_) {
      pImage->CopyLine(loop_index_, loop_index_ - 1);
    } else {
      uint32_t line1 = pImage->GetPixel(1, loop_index_ - 2);
      line1 |= pImage->GetPixel(0, loop_index_ - 2) << 1;
      uint32_t line2 = pImage->GetPixel(2, loop_index_ - 1);
      line2 |= pImage->GetPixel(1, loop_index_ - 1) << 1;
      line2 |= pImage->GetPixel(0, loop_index_ - 1) << 2;
      uint32_t line3 = 0;
      for (uint32_t w = 0; w < GBW; w++) {
        int bVal;
        if (USESKIP && SKIP->GetPixel(w, loop_index_)) {
          bVal = 0;
        } else {
          uint32_t CONTEXT = line3;
          CONTEXT |= pImage->GetPixel(w + GBAT[0], loop_index_ + GBAT[1]) << 4;
          CONTEXT |= line2 << 5;
          CONTEXT |= pImage->GetPixel(w + GBAT[2], loop_index_ + GBAT[3]) << 10;
          CONTEXT |= pImage->GetPixel(w + GBAT[4], loop_index_ + GBAT[5]) << 11;
          CONTEXT |= line1 << 12;
          CONTEXT |= pImage->GetPixel(w + GBAT[6], loop_index_ + GBAT[7]) << 15;
          if (pArithDecoder->IsComplete()) {
            return FXCODEC_STATUS::kError;
          }

          bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
        }
        if (bVal) {
          pImage->SetPixel(w, loop_index_, bVal);
        }
        line1 =
            ((line1 << 1) | pImage->GetPixel(w + 2, loop_index_ - 2)) & 0x07;
        line2 =
            ((line2 << 1) | pImage->GetPixel(w + 3, loop_index_ - 1)) & 0x1f;
        line3 = ((line3 << 1) | bVal) & 0x0f;
      }
    }
    if (pState->pPause && pState->pPause->NeedToPauseNow()) {
      loop_index_++;
      progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
      return FXCODEC_STATUS::kDecodeToBeContinued;
    }
  }
  progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
  return FXCODEC_STATUS::kDecodeFinished;
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate1Opt3(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  if (!line_) {
    line_ = pImage->data();
  }
  int32_t nStride = pImage->stride();
  int32_t nStride2 = nStride << 1;
  int32_t nLineBytes = ((GBW + 7) >> 3) - 1;
  int32_t nBitsLeft = GBW - (nLineBytes << 3);

  UNSAFE_TODO({
    for (; loop_index_ < GBH; loop_index_++) {
      if (TPGDON) {
        if (pArithDecoder->IsComplete()) {
          return FXCODEC_STATUS::kError;
        }

        ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x0795]);
      }
      if (ltp_) {
        pImage->CopyLine(loop_index_, loop_index_ - 1);
      } else {
        if (loop_index_ > 1) {
          uint8_t* pLine1 = line_ - nStride2;
          uint8_t* pLine2 = line_ - nStride;
          uint32_t line1 = (*pLine1++) << 4;
          uint32_t line2 = *pLine2++;
          uint32_t CONTEXT = (line1 & 0x1e00) | ((line2 >> 1) & 0x01f8);
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            line1 = (line1 << 8) | ((*pLine1++) << 4);
            line2 = (line2 << 8) | (*pLine2++);
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x0efb) << 1) | bVal |
                        ((line1 >> k) & 0x0200) | ((line2 >> (k + 1)) & 0x0008);
            }
            line_[cc] = cVal;
          }
          line1 <<= 8;
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x0efb) << 1) | bVal |
                      ((line1 >> (7 - k)) & 0x0200) |
                      ((line2 >> (8 - k)) & 0x0008);
          }
          line_[nLineBytes] = cVal1;
        } else {
          uint8_t* pLine2 = line_ - nStride;
          uint32_t line2 = (loop_index_ & 1) ? (*pLine2++) : 0;
          uint32_t CONTEXT = (line2 >> 1) & 0x01f8;
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            if (loop_index_ & 1) {
              line2 = (line2 << 8) | (*pLine2++);
            }
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x0efb) << 1) | bVal |
                        ((line2 >> (k + 1)) & 0x0008);
            }
            line_[cc] = cVal;
          }
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x0efb) << 1) | bVal |
                      ((line2 >> (8 - k)) & 0x0008);
          }
          line_[nLineBytes] = cVal1;
        }
      }
      line_ += nStride;
      if (pState->pPause && pState->pPause->NeedToPauseNow()) {
        loop_index_++;
        progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
        return FXCODEC_STATUS::kDecodeToBeContinued;
      }
    }
    progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
    return FXCODEC_STATUS::kDecodeFinished;
  });
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate1Unopt(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  for (uint32_t h = 0; h < GBH; h++) {
    if (TPGDON) {
      if (pArithDecoder->IsComplete()) {
        return FXCODEC_STATUS::kError;
      }

      ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x0795]);
    }
    if (ltp_) {
      pImage->CopyLine(h, h - 1);
    } else {
      uint32_t line1 = pImage->GetPixel(2, h - 2);
      line1 |= pImage->GetPixel(1, h - 2) << 1;
      line1 |= pImage->GetPixel(0, h - 2) << 2;
      uint32_t line2 = pImage->GetPixel(2, h - 1);
      line2 |= pImage->GetPixel(1, h - 1) << 1;
      line2 |= pImage->GetPixel(0, h - 1) << 2;
      uint32_t line3 = 0;
      for (uint32_t w = 0; w < GBW; w++) {
        int bVal;
        if (USESKIP && SKIP->GetPixel(w, h)) {
          bVal = 0;
        } else {
          uint32_t CONTEXT = line3;
          CONTEXT |= pImage->GetPixel(w + GBAT[0], h + GBAT[1]) << 3;
          CONTEXT |= line2 << 4;
          CONTEXT |= line1 << 9;
          if (pArithDecoder->IsComplete()) {
            return FXCODEC_STATUS::kError;
          }

          bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
        }
        if (bVal) {
          pImage->SetPixel(w, h, bVal);
        }
        line1 = ((line1 << 1) | pImage->GetPixel(w + 3, h - 2)) & 0x0f;
        line2 = ((line2 << 1) | pImage->GetPixel(w + 3, h - 1)) & 0x1f;
        line3 = ((line3 << 1) | bVal) & 0x07;
      }
    }
    if (pState->pPause && pState->pPause->NeedToPauseNow()) {
      loop_index_++;
      progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
      return FXCODEC_STATUS::kDecodeToBeContinued;
    }
  }
  progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
  return FXCODEC_STATUS::kDecodeFinished;
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate2Opt3(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  if (!line_) {
    line_ = pImage->data();
  }
  int32_t nStride = pImage->stride();
  int32_t nStride2 = nStride << 1;
  int32_t nLineBytes = ((GBW + 7) >> 3) - 1;
  int32_t nBitsLeft = GBW - (nLineBytes << 3);
  UNSAFE_TODO({
    for (; loop_index_ < GBH; loop_index_++) {
      if (TPGDON) {
        if (pArithDecoder->IsComplete()) {
          return FXCODEC_STATUS::kError;
        }

        ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x00e5]);
      }
      if (ltp_) {
        pImage->CopyLine(loop_index_, loop_index_ - 1);
      } else {
        if (loop_index_ > 1) {
          uint8_t* pLine1 = line_ - nStride2;
          uint8_t* pLine2 = line_ - nStride;
          uint32_t line1 = (*pLine1++) << 1;
          uint32_t line2 = *pLine2++;
          uint32_t CONTEXT = (line1 & 0x0380) | ((line2 >> 3) & 0x007c);
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            line1 = (line1 << 8) | ((*pLine1++) << 1);
            line2 = (line2 << 8) | (*pLine2++);
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x01bd) << 1) | bVal |
                        ((line1 >> k) & 0x0080) | ((line2 >> (k + 3)) & 0x0004);
            }
            line_[cc] = cVal;
          }
          line1 <<= 8;
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x01bd) << 1) | bVal |
                      ((line1 >> (7 - k)) & 0x0080) |
                      ((line2 >> (10 - k)) & 0x0004);
          }
          line_[nLineBytes] = cVal1;
        } else {
          uint8_t* pLine2 = line_ - nStride;
          uint32_t line2 = (loop_index_ & 1) ? (*pLine2++) : 0;
          uint32_t CONTEXT = (line2 >> 3) & 0x007c;
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            if (loop_index_ & 1) {
              line2 = (line2 << 8) | (*pLine2++);
            }
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x01bd) << 1) | bVal |
                        ((line2 >> (k + 3)) & 0x0004);
            }
            line_[cc] = cVal;
          }
          line2 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x01bd) << 1) | bVal |
                      (((line2 >> (10 - k))) & 0x0004);
          }
          line_[nLineBytes] = cVal1;
        }
      }
      line_ += nStride;
      if (pState->pPause && loop_index_ % 50 == 0 &&
          pState->pPause->NeedToPauseNow()) {
        loop_index_++;
        progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
        return FXCODEC_STATUS::kDecodeToBeContinued;
      }
    }
    progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
    return FXCODEC_STATUS::kDecodeFinished;
  })
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate2Unopt(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  for (; loop_index_ < GBH; loop_index_++) {
    if (TPGDON) {
      if (pArithDecoder->IsComplete()) {
        return FXCODEC_STATUS::kError;
      }

      ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x00e5]);
    }
    if (ltp_) {
      pImage->CopyLine(loop_index_, loop_index_ - 1);
    } else {
      uint32_t line1 = pImage->GetPixel(1, loop_index_ - 2);
      line1 |= pImage->GetPixel(0, loop_index_ - 2) << 1;
      uint32_t line2 = pImage->GetPixel(1, loop_index_ - 1);
      line2 |= pImage->GetPixel(0, loop_index_ - 1) << 1;
      uint32_t line3 = 0;
      for (uint32_t w = 0; w < GBW; w++) {
        int bVal;
        if (USESKIP && SKIP->GetPixel(w, loop_index_)) {
          bVal = 0;
        } else {
          uint32_t CONTEXT = line3;
          CONTEXT |= pImage->GetPixel(w + GBAT[0], loop_index_ + GBAT[1]) << 2;
          CONTEXT |= line2 << 3;
          CONTEXT |= line1 << 7;
          if (pArithDecoder->IsComplete()) {
            return FXCODEC_STATUS::kError;
          }

          bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
        }
        if (bVal) {
          pImage->SetPixel(w, loop_index_, bVal);
        }
        line1 =
            ((line1 << 1) | pImage->GetPixel(w + 2, loop_index_ - 2)) & 0x07;
        line2 =
            ((line2 << 1) | pImage->GetPixel(w + 2, loop_index_ - 1)) & 0x0f;
        line3 = ((line3 << 1) | bVal) & 0x03;
      }
    }
    if (pState->pPause && pState->pPause->NeedToPauseNow()) {
      loop_index_++;
      progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
      return FXCODEC_STATUS::kDecodeToBeContinued;
    }
  }
  progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
  return FXCODEC_STATUS::kDecodeFinished;
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate3Opt3(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  if (!line_) {
    line_ = pImage->data();
  }
  int32_t nStride = pImage->stride();
  int32_t nLineBytes = ((GBW + 7) >> 3) - 1;
  int32_t nBitsLeft = GBW - (nLineBytes << 3);
  UNSAFE_TODO({
    for (; loop_index_ < GBH; loop_index_++) {
      if (TPGDON) {
        if (pArithDecoder->IsComplete()) {
          return FXCODEC_STATUS::kError;
        }

        ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x0195]);
      }
      if (ltp_) {
        pImage->CopyLine(loop_index_, loop_index_ - 1);
      } else {
        if (loop_index_ > 0) {
          uint8_t* pLine1 = line_ - nStride;
          uint32_t line1 = *pLine1++;
          uint32_t CONTEXT = (line1 >> 1) & 0x03f0;
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            line1 = (line1 << 8) | (*pLine1++);
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal |
                        ((line1 >> (k + 1)) & 0x0010);
            }
            line_[cc] = cVal;
          }
          line1 <<= 8;
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal |
                      ((line1 >> (8 - k)) & 0x0010);
          }
          line_[nLineBytes] = cVal1;
        } else {
          uint32_t CONTEXT = 0;
          for (int32_t cc = 0; cc < nLineBytes; cc++) {
            uint8_t cVal = 0;
            for (int32_t k = 7; k >= 0; k--) {
              if (pArithDecoder->IsComplete()) {
                return FXCODEC_STATUS::kError;
              }

              int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
              cVal |= bVal << k;
              CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal;
            }
            line_[cc] = cVal;
          }
          uint8_t cVal1 = 0;
          for (int32_t k = 0; k < nBitsLeft; k++) {
            if (pArithDecoder->IsComplete()) {
              return FXCODEC_STATUS::kError;
            }

            int bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
            cVal1 |= bVal << (7 - k);
            CONTEXT = ((CONTEXT & 0x01f7) << 1) | bVal;
          }
          line_[nLineBytes] = cVal1;
        }
      }
      line_ += nStride;
      if (pState->pPause && pState->pPause->NeedToPauseNow()) {
        loop_index_++;
        progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
        return FXCODEC_STATUS::kDecodeToBeContinued;
      }
    }
    progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
    return FXCODEC_STATUS::kDecodeFinished;
  });
}

FXCODEC_STATUS CJBig2_GRDProc::ProgressiveDecodeArithTemplate3Unopt(
    ProgressiveArithDecodeState* pState) {
  CJBig2_Image* pImage = pState->pImage->get();
  pdfium::span<JBig2ArithCtx> gbContexts = pState->gbContexts;
  CJBig2_ArithDecoder* pArithDecoder = pState->pArithDecoder;
  for (; loop_index_ < GBH; loop_index_++) {
    if (TPGDON) {
      if (pArithDecoder->IsComplete()) {
        return FXCODEC_STATUS::kError;
      }

      ltp_ = ltp_ ^ pArithDecoder->Decode(&gbContexts[0x0195]);
    }
    if (ltp_) {
      pImage->CopyLine(loop_index_, loop_index_ - 1);
    } else {
      uint32_t line1 = pImage->GetPixel(1, loop_index_ - 1);
      line1 |= pImage->GetPixel(0, loop_index_ - 1) << 1;
      uint32_t line2 = 0;
      for (uint32_t w = 0; w < GBW; w++) {
        int bVal;
        if (USESKIP && SKIP->GetPixel(w, loop_index_)) {
          bVal = 0;
        } else {
          uint32_t CONTEXT = line2;
          CONTEXT |= pImage->GetPixel(w + GBAT[0], loop_index_ + GBAT[1]) << 4;
          CONTEXT |= line1 << 5;
          if (pArithDecoder->IsComplete()) {
            return FXCODEC_STATUS::kError;
          }

          bVal = pArithDecoder->Decode(&gbContexts[CONTEXT]);
        }
        if (bVal) {
          pImage->SetPixel(w, loop_index_, bVal);
        }
        line1 =
            ((line1 << 1) | pImage->GetPixel(w + 2, loop_index_ - 1)) & 0x1f;
        line2 = ((line2 << 1) | bVal) & 0x0f;
      }
    }
    if (pState->pPause && pState->pPause->NeedToPauseNow()) {
      loop_index_++;
      progressive_status_ = FXCODEC_STATUS::kDecodeToBeContinued;
      return FXCODEC_STATUS::kDecodeToBeContinued;
    }
  }
  progressive_status_ = FXCODEC_STATUS::kDecodeFinished;
  return FXCODEC_STATUS::kDecodeFinished;
}
