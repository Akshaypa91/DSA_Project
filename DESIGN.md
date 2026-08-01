# MiniGit — Design and Data Structures

This document explains how MiniGit works internally and why each data
structure was chosen. It is written to be usable directly as the technical
section of a DSA project report.

Notation used throughout:

- `n` — number of files tracked in a commit
- `k` — number of entries in the staging area
- `V` — number of commits in the repository (vertices of the DAG)
- `E` — number of parent edges (`V ≤ E ≤ 2V`, since a commit has at most two parents)
- `L` — number of lines in a file being diffed

---

## 1. Overall model

MiniGit is **content-addressed**. Every object is stored under a key derived
from its own bytes, which gives three properties for free:

1. **Deduplication.** Two identical files share one blob, no matter how many
   commits or branches reference them.
2. **Integrity.** The name of an object is a checksum of its content, so
   corruption is detectable.
3. **Immutability.** Nothing is ever rewritten in place, so copying objects
   between repositories (`push`/`pull`) is idempotent and cannot conflict.

The repository is three cooperating pieces:

```
     working tree                index                object store
   (ordinary files)     (FIFO queue of staged      (blobs + commits,
                          path -> hash pairs)       immutable, hashed)
          |                        |                        |
          |------- add ----------->|                        |
                                   |------- commit -------->|
          |<---------------- checkout / merge --------------|
```

---

## 2. Hashing — FNV-1a (64-bit)

**File:** `src/util.c`

```c
uint64_t mg_fnv1a(const void *data, size_t len)
{
    uint64_t h = 1469598103934665603ULL;      /* offset basis */
    for (size_t i = 0; i < len; i++) {
        h ^= ((const unsigned char *)data)[i];
        h *= 1099511628211ULL;                /* prime */
    }
    return h;
}
```

Two roles: object identity, and bucket selection in the hash table.

**Why this and not `hash = hash*31 + c`?** The classic string hash only mixes
the low bits and is designed for short ASCII keys. Object ids must summarise
arbitrary binary content, so the hash needs good avalanche behaviour — one
flipped input bit should change roughly half the output bits. FNV-1a achieves
that with one XOR and one multiply per byte, and the multiply propagates
changes upward through all 64 bits.

The length is folded into the final value so that a hash over concatenated
fields cannot be confused by re-splitting them:

```c
h ^= (uint64_t)len * FNV_PRIME;
```

**Complexity:** O(len) time, O(1) space.

**Trade-off:** 64 bits is not cryptographic. Git uses SHA-1/SHA-256 to defend
against deliberate collisions. At project scale (thousands of objects) the
birthday bound puts accidental collision probability far below any other
failure mode, and FNV-1a is short enough to read and explain in full.

---

## 3. Hash table with separate chaining

**File:** `src/hashtable.c`

```c
typedef struct HEntry {
    char          *key;
    char          *val;
    struct HEntry *next;   /* collision chain */
} HEntry;

typedef struct {
    HEntry **bucket;
    size_t   nbuckets;
    size_t   count;
} HashMap;
```

Each bucket holds a singly linked list of entries whose keys hash to that
index — so this structure is a hash table *and* a linked list exercise at once.

**Growth policy.** When `count / nbuckets` would exceed 0.75, the table doubles
and every entry is rehashed. Doubling makes the amortised insert cost O(1):
across `m` inserts, the total rehashing work is `1 + 2 + 4 + … + m < 2m`.

**Load factor and chain length.** With a good hash, keys distribute uniformly,
so the expected chain length is exactly the load factor α = count/nbuckets.
Capping α at 0.75 keeps the average probe under one comparison.

| Operation | Average | Worst case |
|---|---|---|
| `hm_put` | O(1) amortised | O(n) — all keys collide |
| `hm_get` / `hm_has` | O(1) | O(n) |
| `hm_del` | O(1) | O(n) |
| `hm_keys` | O(n log n) (sorted output) | O(n log n) |
| Space | O(n) | O(n) |

**Why chaining rather than open addressing?** Deletion is the deciding factor.
Open addressing needs tombstones and careful probe-sequence repair on delete;
chaining just unlinks a node. MiniGit deletes keys during merges
(`hm_del` when a path is removed), so chaining keeps the code honest and short.

**Three uses of one structure:**

| Use | Key | Value |
|---|---|---|
| Commit tree | file path | blob hash |
| Visited set (DAG walks) | commit id | `"1"` |
| Depth memo | commit id | generation number |

---

## 4. FIFO queue — the staging area

**File:** `src/repo.c`

```c
typedef struct IndexEntry {
    char               hash[17];
    char               path[1024];
    struct IndexEntry *next;
} IndexEntry;

typedef struct {
    IndexEntry *front;   /* dequeue end */
    IndexEntry *rear;    /* enqueue end */
    size_t      count;
} Index;
```

`add` enqueues at the rear, `commit` drains from the front in insertion order.
Keeping an explicit `rear` pointer is what makes enqueue O(1) rather than O(k)
— without it every `add` would walk the whole list.

**Idempotent staging.** Re-adding a path must not create a duplicate entry, so
`index_enqueue` first scans for an existing node and updates it in place,
preserving its position. That scan is O(k); a companion hash table would make
it O(1), but `k` is the number of files staged for one commit, and the linear
scan keeps the queue a genuine queue rather than a hybrid.

| Operation | Cost |
|---|---|
| `index_enqueue` (new path) | O(k) to check duplicates, O(1) to link |
| `index_remove` | O(k) |
| `index_save` / `index_load` | O(k) |

The queue is persisted as `.minigit/index`, one `<hash> <path>` line per entry,
so the ordering survives between process invocations.

---

## 5. The commit DAG

**File:** `src/object.c`

```c
typedef struct Commit {
    char   id[17];
    char   parent[17];    /* "" when root      */
    char   parent2[17];   /* "" unless a merge */
    char   message[1024];
    time_t timestamp;
    struct Commit *next;  /* history list linkage */
} Commit;
```

With one parent the history is a **singly linked list**. A merge commit adds a
second parent, which turns it into a **directed acyclic graph**. It cannot
contain a cycle because a commit's id is the hash of its content *including its
parent ids* — an ancestor would have to know its own hash before it was
computed.

### 5.1 Generation numbers (topological depth)

```
depth(root)   = 0
depth(commit) = 1 + max(depth of its parents)
```

This is the length of the longest path back to a root, and a child's depth is
always strictly greater than either parent's — so depth is a valid topological
rank.

**Why it exists.** Timestamps have one-second resolution. Several commits made
in quick succession (very common in a test script) compare *equal*, and any
ordering or ancestor-selection decision made on timestamps alone then becomes
arbitrary. This was a real bug during development: `merge` would occasionally
pick the wrong merge base and build a redundant merge commit instead of
fast-forwarding. Depth breaks those ties correctly.

**Implementation note.** `commit_depths` uses an explicit stack rather than
recursion. A repository with thousands of linear commits would otherwise
recurse thousands of frames deep and risk overflowing the C stack. The
iterative version pushes unresolved parents, revisits the node once they are
memoised, and never grows the C stack at all.

**Complexity:** O(V + E) time, O(V) space, each commit resolved exactly once.

### 5.2 Reachability — `commit_ancestors`

Breadth-first search from a starting commit, using a hash table as the visited
set. Without it, a diamond-shaped history would revisit the shared tail once
per path, and a chain of `d` diamonds would cost O(2^d).

**Complexity:** O(V + E) time, O(V) space.

### 5.3 Merge base — lowest common ancestor

```
A ← ancestors(ours)          BFS
B ← ancestors(theirs)        BFS
common ← A ∩ B               hash lookups, O(|B|)
base   ← the element of `common` with the greatest depth
```

The deepest common ancestor is a true LCA: no other common ancestor can be its
descendant, because a descendant would have strictly greater depth and would
have been selected instead.

**Complexity:** O(V + E) time, O(V) space.

**Alternatives considered.** Binary lifting gives O(log V) queries after
O(V log V) preprocessing, but only on trees, and it would need rebuilding on
every commit. For repositories of this size, two linear traversals are both
faster in practice and far easier to verify.

### 5.4 History listing

`commit_history` performs a BFS from HEAD and inserts each commit into a sorted
linked list, ordered by timestamp descending with depth as the tie-breaker, so
a child is never printed below its own parent.

**Complexity:** O(V + E) for the traversal, O(V²) worst case for the sorted
insertion — acceptable for `log`, which is bounded by what a human will read.

---

## 6. Diff — longest common subsequence

**File:** `src/diff.c`

The LCS of two line sequences is exactly the set of unchanged lines; everything
in `A` outside it was deleted, and everything in `B` outside it was inserted.

```
            ⎧ 0                                    if i = 0 or j = 0
dp[i][j] =  ⎨ dp[i-1][j-1] + 1                     if A[i-1] = B[j-1]
            ⎩ max(dp[i-1][j], dp[i][j-1])          otherwise
```

Fill the table row by row, then walk backwards from `dp[n][m]` to recover the
edit script.

**Complexity:** O(n·m) time, O(n·m) space, O(n+m) for the backtrack.

### Three deliberate changes from the textbook version

**1. Contiguous heap allocation instead of a 2-D stack array.**
A `char dp[1000][1000]` style declaration puts megabytes on the stack and
overflows on modest inputs. The table is one `calloc` block indexed manually:

```c
dp[i * cols + j]
```

This also improves cache locality — row traversal walks contiguous memory.

**2. Iterative backtrack instead of recursion.**
The recursive reconstruction recurses once per output line, so a 50,000-line
file means 50,000 stack frames. The iterative version walks from `(n, m)` to
`(0, 0)` collecting operations, then reverses the array.

**3. A size guard.**
Memory is quadratic, so a pair of 100,000-line files would request 40 GB. Past
40 million cells the diff degrades to a summary rather than attempting the
allocation.

### Output

Three lines of context are kept around each change; longer unchanged runs
collapse to a `@@ ...` marker. Marking which lines to keep is a linear pass
over the op list.

---

## 7. Dynamic arrays

**File:** `src/util.c`

`Buffer` (bytes) and `StrList` (owned strings) both double their capacity when
full. The doubling is what makes append O(1) amortised: `m` appends copy at
most `1 + 2 + 4 + … + m < 2m` bytes in total.

`Buffer` is used to build commit objects and conflict files before a single
atomic write, which avoids repeated `realloc` for every field and keeps partial
writes off disk.

---

## 8. Algorithm summary

| Operation | Algorithm | Time | Space |
|---|---|---|---|
| `add` one file | hash content, dedupe, enqueue | O(size + k) | O(size) |
| `commit` | overlay index onto parent tree | O(n + k) | O(n) |
| `log` | BFS + sorted insert | O(V + E + V²) | O(V) |
| `status` | hash working files, compare | O(n·size) | O(n) |
| `checkout` | tree diff, write files | O(n·size) | O(n) |
| `merge base` | two BFS + intersection | O(V + E) | O(V) |
| `merge` | three-way tree compare | O(n) + diff cost | O(n) |
| `diff` two files | LCS dynamic programming | O(L²) | O(L²) |
| `branch` create | write one ref file | O(1) | O(1) |

---

## 9. Correctness and safety measures

**Memory.** Every allocation goes through `mg_malloc` / `mg_calloc` /
`mg_realloc`, which abort on failure rather than returning NULL into code that
does not check it. The full test suite runs clean under AddressSanitizer,
UndefinedBehaviorSanitizer and LeakSanitizer.

**Bounded strings.** All fixed buffers are written with `snprintf` and sized
against `MG_PATH_MAX` / `MG_NAME_MAX`. The build enables `-Wformat-truncation`
among others, and compiles warning-free under
`-Wall -Wextra -Wpedantic -Wshadow -Wcast-qual -Wwrite-strings -Werror`.

**No shell invocation.** `push` and `pull` copy directories with `opendir` /
`readdir` and ordinary file I/O. An earlier design used
`system("cp -r …")`, which breaks on paths containing spaces and executes
attacker-controlled text if a path is ever untrusted.

**Deterministic object ids.** Tree entries are sorted before serialisation, so
the same logical commit always produces the same id regardless of the order
files were staged in.

**Refusal to destroy work.** `checkout` and `merge` stop when the working tree
has uncommitted changes (overridable with `-f`); `push` refuses to advance a
remote whose head is not an ancestor of the local one.

---

## 10. Testing strategy

`tests/run_tests.sh` runs 76 checks against the real binary in a temporary
directory — no mocks, no framework, only a POSIX shell.

Coverage by area:

| Area | What is verified |
|---|---|
| init | Layout created, re-init is safe |
| add / status | Staging, untracked detection, missing paths rejected |
| commit | Snapshot recorded, empty commits refused |
| content addressing | Identical content produces exactly one blob |
| diff | Line-level insert/delete, commit-to-commit, file-to-file |
| rm | Deletion staged, file removed, untracked rejected |
| branch / checkout | Creation, listing, switching, dirty-tree protection |
| merge | Fast-forward, true three-way, conflict markers, resolution |
| push / pull | Round-trip, idempotence, divergence refused |
| ignore rules | Patterns and directory prefixes honoured |
| errors | Operations outside a repo, unknown commands, bad revisions |

`tests/valgrind_check.sh` re-runs a representative workflow under valgrind and
fails on any invalid access or definite leak.

---

## 11. Known limitations

| Limitation | Consequence | How it would be fixed |
|---|---|---|
| 64-bit hash | Not collision-resistant against an adversary | Use SHA-256 |
| Flat trees | Directory rename rewrites every entry below it | Nested tree objects |
| Whole-file blobs | Large binaries stored in full per version | Delta compression, packfiles |
| Whole-file conflicts | Markers wrap the entire file | Hunk-level diff3 merge |
| Local remotes only | No collaboration over a network | SSH or HTTP transport |
| O(V²) log insertion | Slow on very large histories | Priority queue |
| No `.minigit` locking | Concurrent invocations could interleave writes | Lock file on the index |
