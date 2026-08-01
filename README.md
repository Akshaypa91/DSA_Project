# MiniGit

A small version control system written from scratch in C11, built to put
classical data structures to work on a problem where they actually matter.

It is content-addressed like Git: file contents are stored once under the hash
of their bytes, commits are immutable objects that name a tree and their
parents, and branches are just names pointing into that graph.

```
$ mini_git init
Initialised empty MiniGit repository in .minigit/ (branch 'main')

$ echo "hello" > notes.txt
$ mini_git add notes.txt
Staged 1 file(s).

$ mini_git commit -m "first commit"
[main 4f2a9c01] first commit
 1 file(s) tracked, 1 change(s)
```

---

## Build

Requires a C11 compiler and `make`. No external dependencies.

```bash
make            # build ./mini_git
make test       # build, then run the end-to-end suite (76 checks)
make debug      # rebuild with AddressSanitizer + UndefinedBehaviorSanitizer
make clean
```

Tested with GCC and Clang on Linux and macOS. The only POSIX-specific code is
isolated in `src/util.c`, which also carries a `_WIN32` path for MinGW.

---

## Commands

| Command | Description |
|---|---|
| `init` | Create an empty repository in `./.minigit` |
| `add <path>... \| .` | Stage files (recurses into directories) |
| `rm [--cached] <path>...` | Stage a deletion, and drop the file unless `--cached` |
| `status` | Staged, unstaged and untracked changes |
| `commit -m "msg"` | Record the staged snapshot |
| `log [--oneline] [-n N]` | History, newest first |
| `show [<commit>]` | One commit and the paths it touched |
| `branch [<name> \| -d <name>]` | List, create or delete branches |
| `checkout [-b] [-f] <branch\|commit>` | Switch branches, or detach onto a commit |
| `merge <branch\|commit>` | Fast-forward or three-way merge |
| `diff [<rev> [<rev>]] \| <fileA> <fileB>` | Line differences via LCS |
| `push <path>` / `pull <path>` | Sync objects and refs with another repository |
| `help [<command>]` | Usage |

`st`, `br` and `co` work as aliases for `status`, `branch` and `checkout`.

Revisions can be given as a branch name, a full commit id, or any unambiguous
prefix of one (`mini_git show 4f2a9c01`).

---

## Walkthrough

### Everyday workflow

```bash
mini_git init
echo "print('hi')" > app.py
mini_git add app.py
mini_git commit -m "add app"

vim app.py
mini_git diff             # what changed since HEAD
mini_git add app.py
mini_git commit -m "tweak output"
mini_git log --oneline
```

### Branching and merging

```bash
mini_git checkout -b feature
echo "extra" > feature.txt
mini_git add feature.txt
mini_git commit -m "start feature"

mini_git checkout main
mini_git merge feature
```

When both branches changed the same file, the merge stops and writes the usual
markers into the working file:

```
<<<<<<< ours
our version
=======
their version
>>>>>>> feature
```

Fix the file, `mini_git add` it, and `mini_git commit` — the result is recorded
as a merge commit with two parents.

### Sharing with another repository

```bash
mini_git push /path/to/backup     # copy objects, advance the remote branch
mini_git pull /path/to/backup     # fetch objects, fast-forward
```

Pushes that would overwrite unrelated history are refused; you are told to pull
first. Objects are immutable and content-addressed, so copying them is always
safe to repeat.

### Ignoring files

Create a `.minigitignore`:

```
*.log
build/
secrets.txt
```

Dot-files, `.minigit/`, `.git/`, `build/` and the `mini_git` binary itself are
always ignored.

---

## Repository layout

```
.minigit/
├── HEAD                    "ref: main", or "commit: <id>" when detached
├── MERGE_HEAD              present only during an unresolved merge
├── index                   staging queue: "<hash> <path>" per line
├── refs/heads/<name>       one commit id per branch
└── objects/
    ├── blobs/<xx>/<rest>   file contents, deduplicated by hash
    └── commits/<id>        commit metadata (parents, time, message, tree)
```

A commit object is plain text and readable with `cat`:

```
parent  a1b2c3d4e5f60718
merge   -
time    1754006400
message add app
tree
9f86d081884c7d65 app.py
```

Because the id is the hash of exactly that text, a commit's identity covers its
entire history — changing anything upstream changes every id downstream.

---

## Source layout

```
include/minigit.h     all public declarations
src/
  util.c              hashing, buffers, string lists, filesystem, colour
  hashtable.c         separate-chaining hash map
  repo.c              HEAD, refs, staging queue, merge state
  object.c            blobs, commit objects, DAG traversal, merge base
  diff.c              LCS line diff
  cmd_basic.c         init, add, rm, status
  cmd_history.c       commit, log, show
  cmd_branch.c        branch, checkout
  cmd_merge.c         merge
  cmd_diff.c          diff
  cmd_remote.c        push, pull
  main.c              command table and dispatch
tests/
  run_tests.sh        76 end-to-end checks
  valgrind_check.sh   memory checking over a representative workflow
```

---

## Data structures

The short version; [DESIGN.md](DESIGN.md) has the full write-up with
complexity analysis.

| Structure | Where it is used |
|---|---|
| Hash table (separate chaining) | Object store keys, commit trees, visited sets |
| FIFO queue | The staging area — `add` enqueues, `commit` drains |
| Singly linked list | Commit history, hash buckets, index entries |
| Directed acyclic graph | Commit parentage; BFS finds the merge base |
| Dynamic programming | Longest common subsequence for `diff` |
| Dynamic array | Growable buffers and string lists |

---

## Testing

```bash
make test                             # 76 checks
VERBOSE=1 sh tests/run_tests.sh       # show each command's output
make debug && sh tests/run_tests.sh   # under ASan + UBSan
sh tests/valgrind_check.sh            # leak check (needs valgrind)
```

The suite drives the real binary in a temporary directory and covers
initialisation, staging, commits, content deduplication, diffs, branching,
checkout safety, fast-forward merges, three-way merges, conflict resolution,
push/pull round-trips, divergence detection, ignore rules and error paths.

CI runs the same suite on Linux and macOS against both GCC and Clang, plus
sanitizer and valgrind jobs.

---

## Known limits

These are deliberate simplifications, not oversights:

- **64-bit hashes.** Object ids are 16 hex digits, not SHA-1's 40. Collisions
  are astronomically unlikely at project scale but not cryptographically ruled
  out.
- **Flat trees.** A commit stores a flat path-to-blob list rather than nested
  tree objects, so a directory rename rewrites every entry beneath it.
- **Whole-file blobs.** No delta compression or packfiles.
- **Text-oriented merge.** Conflicts are marked at whole-file granularity.
- **Local remotes only.** `push`/`pull` take a filesystem path; there is no
  network transport.

---

## Licence

See [LICENSE](LICENSE).

DSA course project, second year (2025–2026).
