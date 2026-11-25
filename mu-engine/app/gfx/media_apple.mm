// melee-unlocked's linux_media pictures and videos (port/linux/full/linux_media.h, which upstream answers with FFmpeg on Linux)
// on macOS's own decoders, for its video_background.cpp (menu video backgrounds and border art, gfx/gen_gfx.sh): a video's
// frames through AVFoundation (AVAssetReader, H.264 and HEVC in MP4 or MOV, decoded by VideoToolbox), a picture through
// ImageIO (PNG, JPEG, BMP, HEIC, WebP). decode_audio stays fe/media_decode.cpp's.
// SPDX-License-Identifier: GPL-3.0-or-later
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <ImageIO/ImageIO.h>
#include "full/linux_media.h"   // melee-unlocked port/linux/full (the fetched tree)

#include <algorithm>
#include <cmath>
#include <cstring>

namespace linux_media {

bool load_picture(const std::filesystem::path& path, uint32_t max_w, uint32_t max_h, uint32_t* width, uint32_t* height,
                  std::vector<uint8_t>& bgra) {
  @autoreleasepool {
    NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.string().c_str()]];
    CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)url, nullptr);
    if (!source) return false;
    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    CFRelease(source);
    if (!image) return false;
    size_t w = CGImageGetWidth(image), h = CGImageGetHeight(image);
    if (!w || !h) { CGImageRelease(image); return false; }
    if (max_w && max_h && (w > max_w || h > max_h)) {
      const double fit = std::min(double(max_w) / double(w), double(max_h) / double(h));
      w = std::max<size_t>(1, size_t(double(w) * fit)); h = std::max<size_t>(1, size_t(double(h) * fit));
    }
    bgra.assign(w * h * 4, 0);
    CGColorSpaceRef rgb = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    // BGRA in memory: 32-bit little-endian with the alpha first in the word (premultiplied; the pictures are opaque art).
    CGContextRef ctx = CGBitmapContextCreate(bgra.data(), w, h, 8, w * 4, rgb, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(rgb);
    if (!ctx) { CGImageRelease(image); return false; }
    CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
    CGContextDrawImage(ctx, CGRectMake(0, 0, CGFloat(w), CGFloat(h)), image);   // rows come out top first
    CGContextRelease(ctx);
    CGImageRelease(image);
    *width = uint32_t(w); *height = uint32_t(h);
    return true;
  }
}

struct VideoReader::Impl {
  AVURLAsset* asset = nil;
  AVAssetTrack* track = nil;
  AVAssetReader* reader = nil;
  AVAssetReaderTrackOutput* output = nil;
  uint32_t width = 0, height = 0;
  bool start(std::string* error) {
    NSError* e = nil;
    reader = [[AVAssetReader alloc] initWithAsset:asset error:&e];
    if (!reader) { if (error) *error = e ? e.localizedDescription.UTF8String : "the file cannot be read"; return false; }
    NSDictionary* settings = @{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
    output = [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:settings];
    output.alwaysCopiesSampleData = NO;
    if (![reader canAddOutput:output]) { if (error) *error = "the video track cannot be decoded"; return false; }
    [reader addOutput:output];
    if (![reader startReading]) {
      if (error) *error = reader.error ? reader.error.localizedDescription.UTF8String : "decoding did not start";
      return false;
    }
    return true;
  }
};

VideoReader::VideoReader() : impl_(new Impl) {}
VideoReader::~VideoReader() { @autoreleasepool { if (impl_->reader) [impl_->reader cancelReading]; impl_.reset(); } }

bool VideoReader::open(const std::filesystem::path& path, std::string* error) {
  @autoreleasepool {
    NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.string().c_str()]];
    impl_->asset = [AVURLAsset URLAssetWithURL:url options:@{AVURLAssetPreferPreciseDurationAndTimingKey: @YES}];
    // The synchronous track list (deprecated, still answered): the reader thread may block here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSArray<AVAssetTrack*>* tracks = [impl_->asset tracksWithMediaType:AVMediaTypeVideo];
#pragma clang diagnostic pop
    if (tracks.count == 0) { if (error) *error = "no video track"; return false; }
    impl_->track = tracks.firstObject;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const CGSize size = CGSizeApplyAffineTransform(impl_->track.naturalSize, impl_->track.preferredTransform);
#pragma clang diagnostic pop
    impl_->width = uint32_t(std::lround(std::fabs(size.width))); impl_->height = uint32_t(std::lround(std::fabs(size.height)));
    return impl_->start(error);
  }
}
uint32_t VideoReader::width() const { return impl_->width; }
uint32_t VideoReader::height() const { return impl_->height; }

int VideoReader::read(std::vector<uint8_t>& bgra, int64_t* time_100ns, std::string* error) {
  @autoreleasepool {
    if (!impl_->reader) { if (error) *error = "not open"; return -1; }
    CMSampleBufferRef sample = [impl_->output copyNextSampleBuffer];
    if (!sample) {
      if (impl_->reader.status == AVAssetReaderStatusFailed) {
        if (error) *error = impl_->reader.error ? impl_->reader.error.localizedDescription.UTF8String : "decoding failed";
        return -1;
      }
      return 0;   // the end of the file
    }
    CVImageBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
    if (!pixels) { CFRelease(sample); return read(bgra, time_100ns, error); }   // a sample without a picture
    const CMTime t = CMSampleBufferGetPresentationTimeStamp(sample);
    if (time_100ns) *time_100ns = CMTIME_IS_VALID(t) ? int64_t(CMTimeGetSeconds(t) * 1e7) : 0;
    CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    const size_t w = CVPixelBufferGetWidth(pixels), h = CVPixelBufferGetHeight(pixels), stride = CVPixelBufferGetBytesPerRow(pixels);
    const uint8_t* base = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels));
    const uint32_t ow = impl_->width ? impl_->width : uint32_t(w), oh = impl_->height ? impl_->height : uint32_t(h);
    bgra.assign(size_t(ow) * oh * 4, 0);
    // Bottom row first, as the Windows decoder (and upstream's FFmpeg path) hand frames on.
    const size_t cw = std::min<size_t>(w, ow), ch = std::min<size_t>(h, oh);
    for (size_t y = 0; y < ch && base; ++y)
      std::memcpy(bgra.data() + (oh - 1 - y) * size_t(ow) * 4, base + y * stride, cw * 4);
    CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    CFRelease(sample);
    return 1;
  }
}

bool VideoReader::rewind() {
  @autoreleasepool {
    if (impl_->reader) [impl_->reader cancelReading];
    impl_->reader = nil; impl_->output = nil;
    return impl_->asset && impl_->track && impl_->start(nullptr);
  }
}

}  // namespace linux_media
