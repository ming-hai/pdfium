// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_PROGRESSIVE_DECODER_H_
#define CORE_FXCODEC_PROGRESSIVE_DECODER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/jpeg/jpegmodule.h"
#include "core/fxcodec/progressive_decoder_iface.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr_exclusion.h"
#include "core/fxge/dib/cstretchengine.h"
#include "core/fxge/dib/fx_dib.h"

#ifdef PDF_ENABLE_XFA_BMP
#include "core/fxcodec/bmp/bmp_decoder.h"
#endif  // PDF_ENABLE_XFA_BMP

#ifdef PDF_ENABLE_XFA_GIF
#include "core/fxcodec/gif/gif_decoder.h"
#endif  // PDF_ENABLE_XFA_GIF

#ifdef PDF_ENABLE_XFA_PNG
#include "core/fxcodec/png/png_decoder.h"
#endif  // PDF_ENABLE_XFA_PNG

class CFX_DIBitmap;
class IFX_SeekableReadStream;

namespace fxcodec {

class CFX_DIBAttribute;

class Dummy {};  // Placeholder to work around C++ syntax issues

class ProgressiveDecoder final :
#ifdef PDF_ENABLE_XFA_BMP
    public BmpDecoder::Delegate,
#endif  // PDF_ENABLE_XFA_BMP
#ifdef PDF_ENABLE_XFA_GIF
    public GifDecoder::Delegate,
#endif  // PDF_ENABLE_XFA_GIF
#ifdef PDF_ENABLE_XFA_PNG
    public PngDecoder::Delegate,
#endif  // PDF_ENABLE_XFA_PNG
    public Dummy {
 public:
  enum FXCodec_Format {
    FXCodec_Invalid = 0,
    FXCodec_8bppGray = 0x108,
    FXCodec_8bppRgb = 0x008,
    FXCodec_Rgb = 0x018,
    FXCodec_Rgb32 = 0x020,
    FXCodec_Argb = 0x220,
    FXCodec_Cmyk = 0x120
  };

  ProgressiveDecoder();
  ~ProgressiveDecoder();

  FXCODEC_STATUS LoadImageInfo(RetainPtr<IFX_SeekableReadStream> pFile,
                               FXCODEC_IMAGE_TYPE imageType,
                               CFX_DIBAttribute* pAttribute,
                               bool bSkipImageTypeCheck);

  int32_t GetWidth() const { return src_width_; }
  int32_t GetHeight() const { return src_height_; }

  FXDIB_Format GetBitmapFormat() const;

  std::pair<FXCODEC_STATUS, size_t> GetFrames();
  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);

  FXCODEC_STATUS ContinueDecode();

#ifdef PDF_ENABLE_XFA_PNG
  // PngDecoder::Delegate
  bool PngReadHeader(int width,
                     int height,
                     int bpc,
                     int pass,
                     int* color_type,
                     double* gamma) override;
  uint8_t* PngAskScanlineBuf(int line) override;
  void PngFillScanlineBufCompleted(int pass, int line) override;
#endif  // PDF_ENABLE_XFA_PNG

#ifdef PDF_ENABLE_XFA_GIF
  // GifDecoder::Delegate
  uint32_t GifCurrentPosition() const override;
  bool GifInputRecordPositionBuf(uint32_t rcd_pos,
                                 const FX_RECT& img_rc,
                                 pdfium::span<CFX_GifPalette> pal_span,
                                 int32_t trans_index,
                                 bool interlace) override;
  void GifReadScanline(int32_t row_num, pdfium::span<uint8_t> row_buf) override;
#endif  // PDF_ENABLE_XFA_GIF

#ifdef PDF_ENABLE_XFA_BMP
  // BmpDecoder::Delegate
  bool BmpInputImagePositionBuf(uint32_t rcd_pos) override;
  void BmpReadScanline(uint32_t row_num,
                       pdfium::span<const uint8_t> row_buf) override;
#endif  // PDF_ENABLE_XFA_BMP

 private:
  using WeightTable = CStretchEngine::WeightTable;

  enum class TransformMethod : uint8_t {
    kInvalid,
    k8BppGrayToRgbMaybeAlpha,
    k8BppRgbToRgbNoAlpha,
    k8BppRgbToArgb,
    kRgbMaybeAlphaToRgbMaybeAlpha,
    kCmykToRgbMaybeAlpha,
    kArgbToArgb,
  };

#ifdef PDF_ENABLE_XFA_BMP
  bool BmpReadMoreData(ProgressiveDecoderIface::Context* pBmpContext,
                       FXCODEC_STATUS* err_status);
  bool BmpDetectImageTypeInBuffer(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS BmpStartDecode();
  FXCODEC_STATUS BmpContinueDecode();
#endif  // PDF_ENABLE_XFA_BMP

#ifdef PDF_ENABLE_XFA_GIF
  bool GifReadMoreData(FXCODEC_STATUS* err_status);
  bool GifDetectImageTypeInBuffer();
  FXCODEC_STATUS GifStartDecode();
  FXCODEC_STATUS GifContinueDecode();
#endif  // PDF_ENABLE_XFA_GIF

#ifdef PDF_ENABLE_XFA_PNG
  bool PngDetectImageTypeInBuffer(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS PngStartDecode();
  FXCODEC_STATUS PngContinueDecode();
#endif  // PDF_ENABLE_XFA_PNG

#ifdef PDF_ENABLE_XFA_TIFF
  bool TiffDetectImageTypeFromFile(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS TiffContinueDecode();
#endif  // PDF_ENABLE_XFA_TIFF

  bool JpegReadMoreData(FXCODEC_STATUS* err_status);
  bool JpegDetectImageTypeInBuffer(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS JpegStartDecode();
  FXCODEC_STATUS JpegContinueDecode();

  int32_t GetBitsPerPixel() const { return src_components_ * src_bpc_; }

  bool DetectImageType(FXCODEC_IMAGE_TYPE imageType,
                       CFX_DIBAttribute* pAttribute);
  bool ReadMoreData(ProgressiveDecoderIface* pModule,
                    ProgressiveDecoderIface::Context* pContext,
                    FXCODEC_STATUS* err_status);

  void SetTransMethod();

  void ResampleScanline(const RetainPtr<CFX_DIBitmap>& pDeviceBitmap,
                        int32_t dest_line,
                        pdfium::span<uint8_t> src_span,
                        FXCodec_Format src_format);
  void Resample(const RetainPtr<CFX_DIBitmap>& pDeviceBitmap,
                int32_t src_line,
                uint8_t* src_scan,
                FXCodec_Format src_format);

  FXCODEC_STATUS status_ = FXCODEC_STATUS::kDecodeFinished;
  FXCODEC_IMAGE_TYPE image_type_ = FXCODEC_IMAGE_UNKNOWN;
  RetainPtr<IFX_SeekableReadStream> file_;
  RetainPtr<CFX_DIBitmap> device_bitmap_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  DataVector<uint8_t> decode_buf_;
  DataVector<FX_ARGB> src_palette_;
  std::unique_ptr<ProgressiveDecoderIface::Context> jpeg_context_;
#ifdef PDF_ENABLE_XFA_BMP
  std::unique_ptr<ProgressiveDecoderIface::Context> bmp_context_;
#endif  // PDF_ENABLE_XFA_BMP
#ifdef PDF_ENABLE_XFA_GIF
  std::unique_ptr<ProgressiveDecoderIface::Context> gif_context_;
#endif  // PDF_ENABLE_XFA_GIF
#ifdef PDF_ENABLE_XFA_PNG
  std::unique_ptr<ProgressiveDecoderIface::Context> png_context_;
#endif  // PDF_ENABLE_XFA_PNG
#ifdef PDF_ENABLE_XFA_TIFF
  std::unique_ptr<ProgressiveDecoderIface::Context> tiff_context_;
#endif  // PDF_ENABLE_XFA_TIFF
  uint32_t offset_ = 0;
  int scanline_size_ = 0;
  WeightTable weight_horz_;
  int src_width_ = 0;
  int src_height_ = 0;
  int src_components_ = 0;
  int src_bpc_ = 0;
  TransformMethod trans_method_;
  int src_row_ = 0;
  FXCodec_Format src_format_ = FXCodec_Invalid;
  int src_pass_number_ = 0;
  size_t frame_number_ = 0;
  size_t frame_cur_ = 0;
#ifdef PDF_ENABLE_XFA_GIF
  int gif_bg_index_ = 0;
  pdfium::span<CFX_GifPalette> gif_palette_;
  int gif_trans_index_ = -1;
  FX_RECT gif_frame_rect_;
#endif  // PDF_ENABLE_XFA_GIF
#ifdef PDF_ENABLE_XFA_BMP
  bool bmp_is_top_bottom_ = false;
#endif  // PDF_ENABLE_XFA_BMP
};

}  // namespace fxcodec

using ProgressiveDecoder = fxcodec::ProgressiveDecoder;

#endif  // CORE_FXCODEC_PROGRESSIVE_DECODER_H_
