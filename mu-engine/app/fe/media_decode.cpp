// The player's music files for melee-unlocked's jukebox (Use custom music, features/audio.cpp): linux_media::decode_audio,
// which upstream implements with FFmpeg on Linux and Windows does with Media Foundation. From Melee-NX-NetPlay's
// frontend/src/media_decode.cpp (same author): small permissive decoders fetched by mu-engine/fetch.sh into
// build/mu-engine/audio-decoders (pinned by commit and SHA-256, never committed):
//   MP3   minimp3 (lieff/minimp3, CC0 1.0)
//   OGG   stb_vorbis (nothings/stb, public domain or MIT)
//   WAV   dr_wav (mackron/dr_libs, public domain or MIT No Attribution)
//   FLAC  dr_flac (mackron/dr_libs, public domain or MIT No Attribution)
// and on the Mac also the system's AudioToolbox (ExtAudioFile) for M4A/AAC, ALAC, AIFF and CAF, which the Switch leaves out.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "full/linux_media.h"   // melee-unlocked port/linux/full (the fetched tree): the interface jukebox.cpp calls

#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>
#include <AudioToolbox/AudioToolbox.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_NO_STDIO_WARNINGS
#include "minimp3_ex.h"
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_WCHAR
#include "dr_wav.h"
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_WCHAR
#include "dr_flac.h"
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
#undef L
#undef R
#undef C

namespace linux_media {
namespace {
constexpr size_t kChunk = 4096;   // frames per piece handed to the sink

// Interleaved `channels` floats -> interleaved stereo (mono doubled, more than two: the first two).
void to_stereo(const float* in, size_t frames, unsigned channels, std::vector<float>& out) {
  out.resize(frames * 2);
  for (size_t i = 0; i < frames; ++i) {
    const float l = in[i * channels], r = channels > 1 ? in[i * channels + 1] : l;
    out[i * 2] = l; out[i * 2 + 1] = r;
  }
}

// Hands `frames` stereo frames to the sink within the limit; false stops (sink said so, or too long).
bool give(const AudioSink& sink, const float* stereo, size_t frames, size_t max_frames, size_t* written, bool* too_long, bool* stopped) {
  if (*written + frames > max_frames) { *too_long = true; return false; }
  *written += frames;
  if (!sink(stereo, frames)) { *stopped = true; return false; }
  return true;
}

std::string extension(const std::filesystem::path& path) {
  std::string e = path.extension().u8string();
  std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c) { return (char)std::tolower(c); });
  return e;
}
}  // namespace

bool decode_audio(const std::filesystem::path& path, uint32_t* rate, size_t max_frames, const AudioSink& sink,
                  size_t* expected_frames, bool* too_long) {
  *expected_frames = 0;
  *too_long = false;
  const std::string ext = extension(path), file = path.u8string();
  std::vector<float> pcm, stereo;
  size_t written = 0;
  bool stopped = false;
  if (ext == ".mp3") {
    mp3dec_ex_t dec;
    if (mp3dec_ex_open(&dec, file.c_str(), MP3D_SEEK_TO_SAMPLE)) return false;
    const unsigned channels = (unsigned)dec.info.channels;
    *rate = (uint32_t)dec.info.hz;
    if (!channels || *rate < 8000 || *rate > 384000) { mp3dec_ex_close(&dec); return false; }
    *expected_frames = (size_t)(dec.samples / channels);
    if (*expected_frames > max_frames) { *too_long = true; mp3dec_ex_close(&dec); return false; }
    pcm.resize(kChunk * channels);
    for (;;) {
      const size_t got = mp3dec_ex_read(&dec, pcm.data(), pcm.size());
      const size_t frames = got / channels;
      if (frames) { to_stereo(pcm.data(), frames, channels, stereo); if (!give(sink, stereo.data(), frames, max_frames, &written, too_long, &stopped)) break; }
      if (got < pcm.size()) break;   // the end, or an error (dec.last_error)
    }
    const bool error = dec.last_error != 0;
    mp3dec_ex_close(&dec);
    return (stopped || (!*too_long && !error)) && written > 0;
  }
  if (ext == ".ogg") {
    int error = 0;
    stb_vorbis* v = stb_vorbis_open_filename(file.c_str(), &error, nullptr);
    if (!v) return false;
    const stb_vorbis_info info = stb_vorbis_get_info(v);
    *rate = info.sample_rate;
    if (*rate < 8000 || *rate > 384000) { stb_vorbis_close(v); return false; }
    *expected_frames = stb_vorbis_stream_length_in_samples(v);
    if (*expected_frames > max_frames) { *too_long = true; stb_vorbis_close(v); return false; }
    stereo.resize(kChunk * 2);
    for (;;) {
      const int frames = stb_vorbis_get_samples_float_interleaved(v, 2, stereo.data(), (int)stereo.size());
      if (frames <= 0) break;
      if (!give(sink, stereo.data(), (size_t)frames, max_frames, &written, too_long, &stopped)) break;
    }
    stb_vorbis_close(v);
    return (stopped || !*too_long) && written > 0;
  }
  if (ext == ".wav") {
    drwav wav;
    if (!drwav_init_file(&wav, file.c_str(), nullptr)) return false;
    const unsigned channels = wav.channels;
    *rate = wav.sampleRate;
    if (!channels || *rate < 8000 || *rate > 384000) { drwav_uninit(&wav); return false; }
    *expected_frames = (size_t)wav.totalPCMFrameCount;
    if (*expected_frames > max_frames) { *too_long = true; drwav_uninit(&wav); return false; }
    pcm.resize(kChunk * channels);
    for (;;) {
      const size_t frames = (size_t)drwav_read_pcm_frames_f32(&wav, kChunk, pcm.data());
      if (!frames) break;
      to_stereo(pcm.data(), frames, channels, stereo);
      if (!give(sink, stereo.data(), frames, max_frames, &written, too_long, &stopped)) break;
    }
    drwav_uninit(&wav);
    return (stopped || !*too_long) && written > 0;
  }
  if (ext == ".flac") {
    drflac* flac = drflac_open_file(file.c_str(), nullptr);
    if (!flac) return false;
    const unsigned channels = flac->channels;
    *rate = flac->sampleRate;
    if (!channels || *rate < 8000 || *rate > 384000) { drflac_close(flac); return false; }
    *expected_frames = (size_t)flac->totalPCMFrameCount;
    if (*expected_frames > max_frames) { *too_long = true; drflac_close(flac); return false; }
    pcm.resize(kChunk * channels);
    for (;;) {
      const size_t frames = (size_t)drflac_read_pcm_frames_f32(flac, kChunk, pcm.data());
      if (!frames) break;
      to_stereo(pcm.data(), frames, channels, stereo);
      if (!give(sink, stereo.data(), frames, max_frames, &written, too_long, &stopped)) break;
    }
    drflac_close(flac);
    return (stopped || !*too_long) && written > 0;
  }
  if (ext == ".m4a" || ext == ".aac" || ext == ".mp4" || ext == ".aif" || ext == ".aiff" || ext == ".caf" || ext == ".alac") {
    // The Mac's own decoder: ExtAudioFile hands the file out as interleaved stereo floats at its own rate.
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr, (const UInt8*)file.c_str(), (CFIndex)file.size(), false);
    if (!url) return false;
    ExtAudioFileRef af = nullptr;
    const OSStatus opened = ExtAudioFileOpenURL(url, &af);
    CFRelease(url);
    if (opened != noErr || !af) return false;
    AudioStreamBasicDescription in{};
    UInt32 size = sizeof in;
    SInt64 length = 0;
    UInt32 lsize = sizeof length;
    if (ExtAudioFileGetProperty(af, kExtAudioFileProperty_FileDataFormat, &size, &in) != noErr || in.mSampleRate < 8000 || in.mSampleRate > 384000) {
      ExtAudioFileDispose(af); return false;
    }
    *rate = (uint32_t)in.mSampleRate;
    if (ExtAudioFileGetProperty(af, kExtAudioFileProperty_FileLengthFrames, &lsize, &length) == noErr && length > 0) {
      *expected_frames = (size_t)length;
      if (*expected_frames > max_frames) { *too_long = true; ExtAudioFileDispose(af); return false; }
    }
    AudioStreamBasicDescription want{};
    want.mSampleRate = in.mSampleRate; want.mFormatID = kAudioFormatLinearPCM;
    want.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    want.mChannelsPerFrame = 2; want.mBitsPerChannel = 32; want.mFramesPerPacket = 1; want.mBytesPerFrame = 8; want.mBytesPerPacket = 8;
    if (ExtAudioFileSetProperty(af, kExtAudioFileProperty_ClientDataFormat, sizeof want, &want) != noErr) { ExtAudioFileDispose(af); return false; }
    stereo.resize(kChunk * 2);
    bool error = false;
    for (;;) {
      AudioBufferList list{};
      list.mNumberBuffers = 1;
      list.mBuffers[0].mNumberChannels = 2;
      list.mBuffers[0].mDataByteSize = (UInt32)(stereo.size() * sizeof(float));
      list.mBuffers[0].mData = stereo.data();
      UInt32 frames = (UInt32)kChunk;
      if (ExtAudioFileRead(af, &frames, &list) != noErr) { error = true; break; }
      if (!frames) break;
      if (!give(sink, stereo.data(), frames, max_frames, &written, too_long, &stopped)) break;
    }
    ExtAudioFileDispose(af);
    return (stopped || (!*too_long && !error)) && written > 0;
  }
  return false;   // anything else: the disc's song plays
}

}  // namespace linux_media
