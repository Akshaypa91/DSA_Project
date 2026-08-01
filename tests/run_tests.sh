#!/bin/sh
# MiniGit end-to-end test suite.
#
# Runs the real binary against a throwaway repository in a temp directory.
# No framework, no dependencies beyond a POSIX shell.
#
#   sh tests/run_tests.sh          run everything
#   VERBOSE=1 sh tests/run_tests.sh    echo every command's output

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN="$ROOT/mini_git"
NO_COLOR=1
export NO_COLOR

if [ ! -x "$BIN" ]; then
    echo "tests: $BIN not built -- run 'make' first" >&2
    exit 1
fi

PASS=0
FAIL=0
CURRENT=""

WORK=$(mktemp -d 2>/dev/null || mktemp -d -t minigit)
trap 'rm -rf "$WORK"' EXIT INT TERM

g() {
    if [ "${VERBOSE:-0}" = "1" ]; then
        "$BIN" "$@"
    else
        "$BIN" "$@" >/dev/null 2>&1
    fi
}

# capture stdout of a command
gout() { "$BIN" "$@" 2>&1; }

section() {
    CURRENT="$1"
    printf '\n\033[1m%s\033[0m\n' "$1"
}

ok()   { PASS=$((PASS + 1)); printf '  \033[32mok\033[0m   %s\n' "$1"; }
bad()  { FAIL=$((FAIL + 1)); printf '  \033[31mFAIL\033[0m %s\n' "$1"; }

assert() {                       # assert <desc> <cmd...>
    desc=$1; shift
    if "$@" >/dev/null 2>&1; then ok "$desc"; else bad "$desc"; fi
}

refute() {                       # command must fail
    desc=$1; shift
    if "$@" >/dev/null 2>&1; then bad "$desc"; else ok "$desc"; fi
}

assert_file() {                  # file contents equal expected
    desc=$1; file=$2; want=$3
    if [ -f "$file" ] && [ "$(cat "$file")" = "$want" ]; then
        ok "$desc"
    else
        bad "$desc (got '$(cat "$file" 2>/dev/null)', want '$want')"
    fi
}

assert_contains() {              # output contains substring
    desc=$1; needle=$2; shift 2
    if "$@" 2>&1 | grep -q -- "$needle"; then ok "$desc"; else bad "$desc"; fi
}

assert_missing() {
    desc=$1; file=$2
    if [ ! -e "$file" ]; then ok "$desc"; else bad "$desc"; fi
}

# ------------------------------------------------------------------
cd "$WORK" || exit 1

section "init"
assert "init succeeds"                g init
assert "objects dir created"          test -d .minigit/objects/commits
assert "refs dir created"             test -d .minigit/refs/heads
assert "HEAD points at main"          grep -q "ref: main" .minigit/HEAD
assert "re-init is safe"              g init

section "add and status"
printf 'alpha\nbeta\ngamma\n' > a.txt
printf 'one\n' > b.txt
assert_contains "untracked shows a.txt" "a.txt" "$BIN" status
assert "add single file"              g add a.txt
assert_contains "staged shows new file" "new file:  a.txt" "$BIN" status
assert "add the rest"                 g add .
refute "add of a missing path fails"  g add nope.txt

section "commit"
assert "commit succeeds"              g commit -m "first commit"
assert_contains "clean tree after commit" "nothing to commit" "$BIN" status
refute "empty commit is refused"      g commit -m "empty"
assert_contains "log shows the message" "first commit" "$BIN" log
assert_contains "oneline log works"   "first commit" "$BIN" log --oneline

section "content addressing"
FIRST=$(gout log --oneline | head -1 | cut -d' ' -f1)
assert "commit id is non-empty"       test -n "$FIRST"
assert "commit object exists"         test -f ".minigit/objects/commits/$(ls .minigit/objects/commits | head -1)"
BLOBS=$(find .minigit/objects/blobs -type f | wc -l)
assert "two blobs stored"             test "$BLOBS" -eq 2
printf 'alpha\nbeta\ngamma\n' > c.txt
g add c.txt
BLOBS2=$(find .minigit/objects/blobs -type f | wc -l)
assert "identical content is deduplicated" test "$BLOBS2" -eq 2
g commit -m "add duplicate content"

section "modify and diff"
printf 'alpha\nBETA\ngamma\ndelta\n' > a.txt
assert_contains "unstaged modification detected" "modified:  a.txt" "$BIN" status
assert_contains "diff shows the removed line" "^- beta" "$BIN" diff
assert_contains "diff shows the added line"   "^+ BETA" "$BIN" diff
g add a.txt
g commit -m "edit a.txt"
assert_contains "diff of two commits works" "insertion" "$BIN" diff "$FIRST" HEAD

section "file-to-file diff"
printf 'x\ny\n' > f1
printf 'x\nz\n' > f2
assert_contains "plain file diff works" "insertion" "$BIN" diff f1 f2
rm -f f1 f2

section "rm"
assert "rm stages a deletion"         g rm b.txt
assert_missing "file removed from disk" b.txt
assert_contains "status shows deletion" "deleted:   b.txt" "$BIN" status
g commit -m "drop b.txt"
refute "rm of untracked path fails"   g rm b.txt

section "branching"
assert "create branch feature"        g branch feature
assert_contains "branch list shows feature" "feature" "$BIN" branch
refute "duplicate branch is refused"  g branch feature
assert "checkout feature"             g checkout feature
assert_contains "HEAD moved to feature" "On branch feature" "$BIN" status

printf 'feature work\n' > feature.txt
g add feature.txt
g commit -m "feature commit"
assert "feature.txt exists on branch"  test -f feature.txt

assert "checkout main"                g checkout main
assert_missing "feature.txt gone on main" feature.txt
assert "a.txt still present on main"  test -f a.txt

section "checkout safety"
printf 'dirty\n' >> a.txt
refute "dirty checkout is blocked"    g checkout feature
assert "forced checkout works"        g checkout -f feature
assert "back to main"                 g checkout -f main

section "fast-forward merge"
assert "merge feature into main"      g merge feature
assert "feature.txt merged in"        test -f feature.txt
assert_contains "second merge is a no-op" "Already up to date" "$BIN" merge feature

section "three-way merge without conflict"
g checkout -b side
printf 'side file\n' > side.txt
g add side.txt
g commit -m "side commit"
g checkout main
printf 'main only\n' > main_only.txt
g add main_only.txt
g commit -m "main commit"
assert "true merge succeeds"          g merge side
assert "side.txt present"             test -f side.txt
assert "main_only.txt present"        test -f main_only.txt
assert_contains "merge commit recorded" "Merge:" "$BIN" log

section "conflicting merge"
g checkout -b conflict
printf 'THEIRS\n' > shared.txt
g add shared.txt
g commit -m "theirs"
g checkout main
printf 'OURS\n' > shared.txt
g add shared.txt
g commit -m "ours"
refute "merge reports conflict"       g merge conflict
assert_contains "conflict markers written" "<<<<<<< ours" cat shared.txt
assert_contains "theirs side present"  "THEIRS" cat shared.txt
assert "MERGE_HEAD recorded"          test -f .minigit/MERGE_HEAD
printf 'RESOLVED\n' > shared.txt
g add shared.txt
assert "resolution commits"           g commit -m "resolve conflict"
assert_missing "MERGE_HEAD cleared"   .minigit/MERGE_HEAD
assert_file "resolved content kept"   shared.txt "RESOLVED"

section "show"
assert_contains "show HEAD works"     "commit " "$BIN" show
assert_contains "show names the commit message" "resolve conflict" "$BIN" show HEAD

section "commit id prefix resolution"
SHORT=$(gout log --oneline | head -1 | cut -d' ' -f1)
assert_contains "8-char prefix resolves" "commit " "$BIN" show "$SHORT"
refute "bogus revision is rejected"   g show deadbeefdeadbeef

section "push and pull"
mkdir -p "$WORK/remote"
assert "push to a fresh remote"       g push "$WORK/remote"
assert "remote got objects"           test -d "$WORK/remote/.minigit/objects/commits"
assert "remote ref written"           test -f "$WORK/remote/.minigit/refs/heads/main"
assert_contains "push again is idempotent" "Pushed" "$BIN" push "$WORK/remote"

mkdir -p "$WORK/clone"
cd "$WORK/clone" || exit 1
g init
assert "pull from remote"             g pull "$WORK/remote"
assert "pulled shared.txt"            test -f shared.txt
assert_file "pulled content matches"  shared.txt "RESOLVED"
assert_contains "pull again is up to date" "Already up to date" "$BIN" pull "$WORK/remote"

section "push of an ahead branch"
printf 'clone change\n' > clone.txt
g add clone.txt
g commit -m "clone commit"
assert "push fast-forwards the remote" g push "$WORK/remote"

section "divergence is refused"
# Advance the origin repository independently so the two histories fork.
cd "$WORK" || exit 1
printf 'origin side\n' > origin_only.txt
g add origin_only.txt
g commit -m "origin commit"
refute "push of diverged history is refused" g push "$WORK/remote"
assert_contains "pull reports divergence" "diverged" "$BIN" pull "$WORK/remote"
cd "$WORK" || exit 1

section "ignore rules"
cd "$WORK" || exit 1
printf '*.log\nbuild/\n' > .minigitignore
printf 'noise\n' > debug.log
mkdir -p build && printf 'x\n' > build/out.o
OUT=$(gout status)
if printf '%s' "$OUT" | grep -q "debug.log"; then bad "*.log is ignored"; else ok "*.log is ignored"; fi
if printf '%s' "$OUT" | grep -q "build/out.o"; then bad "build/ is ignored"; else ok "build/ is ignored"; fi

section "error handling"
mkdir -p "$WORK/norepo"
cd "$WORK/norepo" || exit 1
refute "status outside a repo fails"  g status
refute "commit outside a repo fails"  g commit -m x
cd "$WORK" || exit 1
refute "unknown command fails"        g frobnicate
assert_contains "help lists commands" "checkout" "$BIN" help
assert_contains "help for one command" "usage: mini_git merge" "$BIN" help merge

# ------------------------------------------------------------------
printf '\n\033[1mResults:\033[0m %d passed, %d failed\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ] || exit 1
