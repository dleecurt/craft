#!/bin/sh
# Plays recorded games back without any window and checks that they play
# out exactly as recorded (the game must be deterministic for network
# play).  Recordings go stale when the game state changes shape; record
# new ones with "craft --record FILE" and replace them.
srcdir=${srcdir:-.}
status=0
cfg=$(mktemp -d)
trap 'rm -rf "$cfg"' EXIT
for r in "$srcdir"/tests/*.craftreplay; do
   out=$(CRAFT_DATADIR="$srcdir" CRAFT_SOUND=off XDG_CONFIG_HOME="$cfg" \
         ./craft --replay "$r" --no-view 2>&1 | grep '^replay:')
   echo "$(basename "$r"): $out"
   case "$out" in
      *"replay: matched"*) ;;
      *) status=1 ;;
   esac
done
exit $status
