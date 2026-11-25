#!/usr/bin/env bash
# Use custom music's decoders (mu-engine/app/fe/media_decode.cpp): test tones (a 3 s 440 Hz sine at half scale) in WAV, MP3,
# OGG Vorbis, FLAC, M4A (AAC) and AIFF made with ffmpeg on this Mac (build/mu-engine/music-test, never committed), then
# tools/music_decode_test.cpp built with the Command Line Tools and run. Needs mu-engine/fetch.sh (the decoders and the
# fetched tree's linux_media.h) and ffmpeg. From Melee-NX-NetPlay's tools/music_decode_test.sh (same author).
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
tones="$repo/build/mu-engine/music-test"; rm -rf "$tones"; mkdir -p "$tones"
s='0.5*sin(2*PI*440*t)'
tone() { ffmpeg -nostdin -loglevel error -f lavfi -i "aevalsrc=$s|$s:s=$1:d=3" "${@:2}"; }
tone 44100 "$tones/tone.wav"
ffmpeg -nostdin -loglevel error -f lavfi -i "aevalsrc=$s:s=22050:d=3" "$tones/tone-mono.wav"
tone 44100 -c:a libmp3lame -b:a 192k "$tones/tone.mp3"
tone 48000 -c:a vorbis -strict experimental "$tones/tone.ogg"
tone 32000 -c:a flac "$tones/tone.flac"
tone 44100 -c:a aac "$tones/tone.m4a"
tone 44100 "$tones/tone.aiff"
cp "$tones/tone.wav" "$tones/tone.xyz"
( unset DEVELOPER_DIR
  clang++ -std=c++17 -O1 -w -o "$tones/music_decode_test" "$here/music_decode_test.cpp" "$repo/mu-engine/app/fe/media_decode.cpp" \
    -I"$repo/build/mu-engine/mu/port/linux" -I"$repo/build/mu-engine/audio-decoders" -framework AudioToolbox -framework CoreFoundation )
"$tones/music_decode_test" "$tones"
