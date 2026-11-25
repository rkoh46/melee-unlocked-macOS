#!/bin/bash
# melee-unlocked's Audio buffer and Use custom music in the app's engine: hidden offline starts of MeleeUnlockedEngine driven by
# a controller script through the app's input path (play_vs_script.py: title, main menu, a VS match on Onett), SDL's disk audio
# driver recording what the output plays (the game's sound and the jukebox's music) instead of the speakers. As
# Melee-NX-NetPlay's frontend/test-audio-linux.sh.
#   app_audio_test.sh ISO CASE...
# Cases:
#   auto      `audio_mode 0` (Auto): the buffer it starts with, what gaps in the match add, the number kept in audio-auto.txt;
#             then a second start, which begins where the first settled
#   fixed     `audio_mode 1`, `audio_buffer_ms 15` (Low latency): the ring's slack stays 15 ms
#   jukebox   `musicpacks 1` and no music pack: the game's own songs through the jukebox (decoded from the disc's HPS files)
#   packs     `musicpacks 1` with test tones (music_decode_test.sh makes them) as the main menu's three songs (a playlist folder
#             MusicPacks/audio/menu01+menu02+menu3/ with an MP3 and a FLAC: a random pick each start) and Onett's two songs (a folder
#             MusicPacks/audio/onetto+onetto2/ with an OGG and an M4A): the log names each pick, and the recording has a 440 Hz
#             tone over the game's sound from the main menu on
#   off       no settings: the game's own music stream as before, and no 440 Hz tone in the recording
#   menusounds launcher.ini `menusounds=1` and the settings panel opened at retrace 300 on CUSTOMIZE (MELEE_MENU_OPEN): the
#             panel's open tick (990 Hz, 55 ms) mixed into the game's output; the recording is searched for 990 Hz in 0.1 s steps
# Output: build/mu-engine/runs/app-audio/<case>/ (log, out.raw, melee-unlocked/MusicPacks/).
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; shift
engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/app-audio"; mkdir -p "$root"
run() {   # $1 folder, $2 frames
  MELEE_UPDATE_CHECK=0 SDL_AUDIO_DRIVER=disk SDL_AUDIO_DISK_OUTPUT_FILE="$1/out.raw" timeout 600 "$engine" --iso "$iso" \
    --profile-dir "$1" --offline --hidden --volume 70 --frames "$2" --no-record --play-script "$root/vs.txt" >> "$1/log" 2>&1
  echo "engine exit $?" >> "$1/log"
}
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$root/vs.txt"
fresh() { rm -rf "$1"; mkdir -p "$1/melee-unlocked"; }
# The recording's 440 Hz share: Goertzel power at 440 Hz against the whole signal, per second (16-bit stereo or 32-bit float).
tone_report() {   # $3: another frequency, looked for in tenths of a second
  python3 -I - "$1" "$2" ${3:-} <<'PY'
import math, struct, sys
path, rate = sys.argv[1], int(sys.argv[2])
data = open(path, "rb").read()
# SDL's disk driver writes the device's format: 32-bit float when the samples look like floats, else 16-bit.
fl = struct.unpack("<%df" % min(len(data) // 4, 200000), data[:min(len(data) // 4, 200000) * 4])
is_float = all(abs(x) <= 1.5 for x in fl if x == x) and any(x != 0 for x in fl)
if is_float:
    n = len(data) // 8; left = [x * 32767 for x in struct.unpack("<%df" % (n * 2), data[:n * 8])[0::2]]
else:
    n = len(data) // 4; left = struct.unpack("<%dh" % (n * 2), data[:n * 4])[0::2]
hz = float(sys.argv[3]) if len(sys.argv) > 3 else 440.0
step = rate // 10 if len(sys.argv) > 3 else rate   # a short tick: tenths of a second
k = 2 * math.cos(2 * math.pi * hz / rate)
seconds = []
for start in range(0, n - step, step):
    s1 = s2 = 0.0; energy = 0.0
    for x in left[start:start + step]:
        s = x + k * s1 - s2; s2 = s1; s1 = s; energy += x * x
    power = s1 * s1 + s2 * s2 - k * s1 * s2
    seconds.append(power / (energy * step / 2) if energy else 0.0)
loud = [i for i, v in enumerate(seconds) if v > 0.2]
heard = [i for i, v in enumerate(seconds) if v == v and v > 0]
unit = "tenths of a second" if step != rate else "seconds"
print("recording (%s): %.1f s at %d Hz, %d %s with sound; %s where a %g Hz tone is over 20%% of the sound: %d (%s)" % (
      "float" if is_float else "16-bit", n / rate, rate, len(heard), unit, unit, hz, len(loud), ", ".join(str(i) for i in loud[:40]) + (" ..." if len(loud) > 40 else "")))
if step != rate and seconds:
    order = sorted(range(len(seconds)), key=lambda i: -seconds[i])[:3]
    print("strongest %g Hz shares: %s; median %.3f" % (hz, ", ".join("%.3f at %.1f s" % (seconds[i], i / 10) for i in order), sorted(seconds)[len(seconds) // 2]))
PY
}
for case in "$@"; do
  dir="$root/$case"; fresh "$dir"; set_ini="$dir/melee-unlocked/settings.ini"
  case "$case" in
  auto)    printf '%s\n' "audio_mode 0" > "$set_ini"; run "$dir" 3000; echo "--- second start" >> "$dir/log"; run "$dir" 900 ;;
  fixed)   printf '%s\n' "audio_mode 1" "audio_buffer_ms 15" > "$set_ini"; run "$dir" 3000 ;;
  jukebox) printf '%s\n' "musicpacks 1" > "$set_ini"; run "$dir" 3000 ;;
  off)     run "$dir" 3000 ;;
  menusounds)
    printf '%s\n' "menusounds=1" > "$dir/launcher.ini"
    MELEE_UPDATE_CHECK=0 MELEE_MENU_OPEN=300 MELEE_MENU_PAGE=CUSTOMIZE SDL_AUDIO_DRIVER=disk SDL_AUDIO_DISK_OUTPUT_FILE="$dir/out.raw" timeout 300 \
      "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 70 --frames 600 --no-record >> "$dir/log" 2>&1
    echo "engine exit $?" >> "$dir/log" ;;
  packs)
    tones="$repo/build/mu-engine/music-test"
    [ -f "$tones/tone.m4a" ] || bash "$here/music_decode_test.sh" > /dev/null
    mkdir -p "$dir/melee-unlocked/MusicPacks/audio/menu01+menu02+menu3" "$dir/melee-unlocked/MusicPacks/audio/onetto+onetto2"
    cp "$tones/tone.mp3" "$tones/tone.flac" "$dir/melee-unlocked/MusicPacks/audio/menu01+menu02+menu3/"
    cp "$tones/tone.ogg" "$tones/tone.m4a" "$dir/melee-unlocked/MusicPacks/audio/onetto+onetto2/"
    printf '%s\n' "musicpacks 1" > "$set_ini"; run "$dir" 3000 ;;
  *) echo "unknown case $case" >&2; exit 2 ;;
  esac
  echo "== $case"
  grep -hE "audio:|jukebox:|play: scene 2/2|engine exit|second start|menu sound|settings: opened" "$dir/log" | grep -v "audio: game made" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | tail -30
  rate=$(sed -n 's/.*device buffer [0-9]* frames at \([0-9]*\) Hz.*/\1/p' "$dir/log" | head -1)
  [ -f "$dir/out.raw" ] && [ -n "$rate" ] && tone_report "$dir/out.raw" "$rate"
  [ "$case" = menusounds ] && [ -f "$dir/out.raw" ] && [ -n "$rate" ] && tone_report "$dir/out.raw" "$rate" 990
  [ -f "$dir/melee-unlocked/audio-auto.txt" ] && { echo "audio-auto.txt:"; cat "$dir/melee-unlocked/audio-auto.txt"; }
  [ -f "$dir/melee-unlocked/MusicPacks/Tracks.txt" ] && echo "MusicPacks/Tracks.txt: $(wc -l < "$dir/melee-unlocked/MusicPacks/Tracks.txt") lines"
  true
done
