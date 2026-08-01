#!/bin/sh
# Run a representative MiniGit workflow under valgrind and fail on any
# invalid access or definite leak.
#
#   sh tests/valgrind_check.sh

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN="$ROOT/mini_git"
NO_COLOR=1
export NO_COLOR

if [ ! -x "$BIN" ]; then
    echo "valgrind_check: $BIN not built -- run 'make' first" >&2
    exit 1
fi

if ! command -v valgrind >/dev/null 2>&1; then
    echo "valgrind_check: valgrind not installed, skipping" >&2
    exit 0
fi

WORK=$(mktemp -d 2>/dev/null || mktemp -d -t minigit-vg)
LOGDIR="$WORK/logs"
mkdir -p "$LOGDIR"
trap 'rm -rf "$WORK"' EXIT INT TERM

STEP=0
FAILED=0

vg() {
    STEP=$((STEP + 1))
    log="$LOGDIR/step-$STEP.log"
    valgrind --error-exitcode=42 \
             --leak-check=full \
             --errors-for-leak-kinds=definite \
             --track-origins=yes \
             --log-file="$log" \
             "$BIN" "$@" >/dev/null 2>&1 || true

    if grep -q "ERROR SUMMARY: [^0]" "$log" 2>/dev/null; then
        printf '  \033[31mFAIL\033[0m valgrind errors in: %s\n' "$*"
        grep -E "ERROR SUMMARY|definitely lost|Invalid " "$log" | sed 's/^/      /'
        FAILED=$((FAILED + 1))
    else
        printf '  \033[32mok\033[0m   %s\n' "$*"
    fi
}

cd "$WORK"

printf '\033[1mvalgrind: core workflow\033[0m\n'
vg init

printf 'alpha\nbeta\ngamma\n' > a.txt
printf 'one\ntwo\n' > b.txt
vg add a.txt b.txt
vg status
vg commit -m "first commit"

printf 'alpha\nBETA\ngamma\ndelta\n' > a.txt
vg diff
vg add a.txt
vg commit -m "second commit"
vg log
vg show
vg log --oneline

printf '\033[1mvalgrind: branching and merging\033[0m\n'
vg branch feature
vg checkout feature
printf 'feature\n' > feature.txt
vg add feature.txt
vg commit -m "feature commit"
vg checkout main
vg merge feature
vg branch

printf '\033[1mvalgrind: conflict path\033[0m\n'
vg checkout -b other
printf 'THEIRS\n' > shared.txt
vg add shared.txt
vg commit -m "theirs"
vg checkout main
printf 'OURS\n' > shared.txt
vg add shared.txt
vg commit -m "ours"
vg merge other
printf 'RESOLVED\n' > shared.txt
vg add shared.txt
vg commit -m "resolve"

printf '\033[1mvalgrind: remote sync\033[0m\n'
mkdir -p "$WORK/remote"
vg push "$WORK/remote"
vg rm b.txt
vg commit -m "drop b"
vg push "$WORK/remote"

printf '\033[1mvalgrind: error paths\033[0m\n'
vg show deadbeefdeadbeef
vg diff nosuchrev1 nosuchrev2
vg merge no-such-branch
vg help
vg help merge

if [ "$FAILED" -ne 0 ]; then
    printf '\n\033[31m%d valgrind check(s) failed\033[0m\n' "$FAILED"
    exit 1
fi

printf '\n\033[32mvalgrind: %d invocations clean\033[0m\n' "$STEP"
