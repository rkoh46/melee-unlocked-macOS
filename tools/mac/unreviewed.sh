#!/bin/zsh
# List the engine's logs and the crash reports nobody has reviewed yet, with the lines that matter.
# The engine writes melee-unlocked/engine.log each start (and keeps the one before as engine.prev.log); a log is known by its
# name and the time it was last written. Logs/session-*.log are the earlier engine's, kept for history.
#   tools/mac/unreviewed.sh              list unreviewed files and their flagged lines
#   tools/mac/unreviewed.sh --mark F...  record files (full paths or basenames) as reviewed
# Review state is local to this Mac: .reviewed-logs in the repo root (git-excluded).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
STATE="$ROOT/.reviewed-logs"
touch "$STATE"
if [[ "${1:-}" == "--mark" ]]; then
  shift
  for f in "$@"; do
    if [[ "$f" == */melee-unlocked/engine*.log && -f "$f" ]]; then echo "$(basename "$f")@$(stat -f %m "$f")"; else basename "$f"; fi
  done >> "$STATE"
  sort -u "$STATE" -o "$STATE"
  exit 0
fi
SUP="$HOME/Library/Application Support/Dashdance"
CRASHES="$HOME/Library/Logs/DiagnosticReports"
# A session still being written is not ready for review.
running=0
pgrep -x MeleeUnlockedEngine > /dev/null && running=1
PATTERN='FATAL|DESYNC|desync|mismatched|force-disconnect|disconnect|connection failed|cannot start|stopped with code [1-9-]|read failed'
found=0
key() { if [[ "$1" == */melee-unlocked/engine*.log ]]; then echo "$(basename "$1")@$(stat -f %m "$1")"; else basename "$1"; fi; }
for f in "$SUP"/melee-unlocked/engine.prev.log(N) "$SUP"/melee-unlocked/engine.log(N) "$SUP"/Logs/session-*.log(N) \
         "$CRASHES"/MeleeUnlockedEngine*.ips(N) "$CRASHES"/melee-unlocked-macOS*.ips(N) "$CRASHES"/Dashdance*.ips(N); do
  name="$(key "$f")"
  grep -qxF "$name" "$STATE" && continue
  (( running )) && [[ "$f" == */engine.log ]] && continue
  found=1
  echo "=== $f"
  if [[ "$f" == *.ips ]]; then
    python3 - "$f" <<'PY'
import json, sys
head, body = open(sys.argv[1]).read().split('\n', 1)
d = json.loads(body); ex = d.get('exception', {})
print(f"crash: {ex.get('type')} {ex.get('signal')} at {json.loads(head).get('timestamp')}")
imgs = d['usedImages']; t = d['threads'][d.get('faultingThread', 0)]
for fr in t['frames'][:16]:
    print('  ', imgs[fr['imageIndex']].get('name', '?'), fr.get('symbol', '?'))
PY
  else
    hits="$(grep -nE "$PATTERN" "$f" | grep -v 'checksums agree' | head -40)"
    if [[ -n "$hits" ]]; then echo "$hits"; else echo "clean: $(grep -m1 -E 'stopped with code|^result:' "$f" || echo 'no end line (killed or crashed)')"; fi
  fi
done
(( found )) || echo "Nothing unreviewed."
