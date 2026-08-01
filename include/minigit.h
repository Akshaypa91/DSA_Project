/*
 * minigit.h - public interface for MiniGit
 *
 * MiniGit is a small, content-addressed version control system written in C.
 * It exists to demonstrate classical data structures on a real workload:
 *
 *   Hash table (separate chaining) .... object store keys, tree maps, visited sets
 *   FIFO queue ....................... the staging area (index)
 *   Singly linked list ............... commit history, branch list, hash buckets
 *   Directed acyclic graph ........... commit parentage; BFS finds the merge base
 *   Dynamic programming (LCS) ........ line diff between two file versions
 *   Dynamic arrays ................... buffers and string lists
 *
 * See DESIGN.md for the full write-up including complexity analysis.
 */
#ifndef MINIGIT_H
#define MINIGIT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MG_VERSION   "2.0.0"
#define MG_DIR       ".minigit"
#define MG_PATH_MAX  1024
#define MG_HASHLEN   16               /* hex digits in an object id */
#define MG_HASHSZ    (MG_HASHLEN + 1) /* + NUL */
#define MG_MSG_MAX   1024
#define MG_NAME_MAX  128
#define MG_DELETED   "0000000000000000" /* index marker: staged deletion */

#if defined(__GNUC__)
#  define MG_PRINTF(a, b) __attribute__((format(printf, a, b)))
#  define MG_NORETURN     __attribute__((noreturn))
#else
#  define MG_PRINTF(a, b)
#  define MG_NORETURN
#endif

/* ------------------------------------------------------------------ *
 * util.c - diagnostics and checked allocation
 * ------------------------------------------------------------------ */
MG_NORETURN void mg_die(const char *fmt, ...) MG_PRINTF(1, 2);
void  mg_warn(const char *fmt, ...) MG_PRINTF(1, 2);
void *mg_malloc(size_t n);
void *mg_calloc(size_t n, size_t sz);
void *mg_realloc(void *p, size_t n);
char *mg_strdup(const char *s);

/* ------------------------------------------------------------------ *
 * util.c - FNV-1a 64-bit hashing (object ids and hash-table indices)
 * ------------------------------------------------------------------ */
uint64_t mg_fnv1a(const void *data, size_t len);
uint64_t mg_fnv1a_str(const char *s);
void     mg_hash_hex(uint64_t h, char out[MG_HASHSZ]);
void     mg_hash_data(const void *data, size_t len, char out[MG_HASHSZ]);

/* ------------------------------------------------------------------ *
 * util.c - growable byte buffer
 * ------------------------------------------------------------------ */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} Buffer;

void buf_init(Buffer *b);
void buf_append(Buffer *b, const void *s, size_t n);
void buf_puts(Buffer *b, const char *s);
void buf_printf(Buffer *b, const char *fmt, ...) MG_PRINTF(2, 3);
void buf_free(Buffer *b);

/* ------------------------------------------------------------------ *
 * util.c - dynamic array of owned strings
 * ------------------------------------------------------------------ */
typedef struct {
    char **item;
    size_t len;
    size_t cap;
} StrList;

void sl_init(StrList *l);
void sl_push(StrList *l, const char *s);
int  sl_has(const StrList *l, const char *s);
void sl_sort(StrList *l);
void sl_free(StrList *l);

/* ------------------------------------------------------------------ *
 * util.c - filesystem helpers (POSIX + Windows)
 * ------------------------------------------------------------------ */
int   mg_mkdir(const char *path);
int   mg_mkdir_p(const char *path);
int   mg_exists(const char *path);
int   mg_is_dir(const char *path);
char *mg_read_file(const char *path, size_t *out_len); /* NUL-terminated; caller frees */
int   mg_write_file(const char *path, const void *data, size_t len);
int   mg_copy_file(const char *src, const char *dst);
int   mg_copy_tree(const char *src, const char *dst);
int   mg_unlink(const char *path);
void  mg_parent_dir(const char *path, char *out, size_t n);
char *mg_trim(char *s);
int   mg_list_dir(const char *path, StrList *out);
void  mg_walk_worktree(StrList *out); /* every trackable file, ignore rules applied */
int   mg_hash_path(const char *path, char out[MG_HASHSZ]);

/* ------------------------------------------------------------------ *
 * hashtable.c - string -> string map, separate chaining
 * Also used as a set (value "1") and as a tree map (path -> blob hash).
 * ------------------------------------------------------------------ */
typedef struct HEntry {
    char          *key;
    char          *val;
    struct HEntry *next; /* collision chain */
} HEntry;

typedef struct {
    HEntry **bucket;
    size_t   nbuckets;
    size_t   count;
} HashMap;

void        hm_init(HashMap *m);
void        hm_put(HashMap *m, const char *key, const char *val);
const char *hm_get(const HashMap *m, const char *key);
int         hm_has(const HashMap *m, const char *key);
void        hm_del(HashMap *m, const char *key);
void        hm_keys(const HashMap *m, StrList *out); /* sorted */
void        hm_free(HashMap *m);
double      hm_load_factor(const HashMap *m);

/* ------------------------------------------------------------------ *
 * repo.c - repository layout, HEAD, refs, staging index
 * ------------------------------------------------------------------ */
int  repo_exists(void);
void repo_require(void);
void repo_path(char *out, size_t n, const char *fmt, ...) MG_PRINTF(3, 4);

int  head_branch(char *out, size_t n);        /* 1 => on a branch */
int  head_commit(char out[MG_HASHSZ]);        /* 1 => HEAD resolves to a commit */
void head_set_branch(const char *branch);
void head_set_detached(const char *commit);
void head_describe(char *out, size_t n);      /* "main" or "HEAD detached at abc123" */

int  ref_read(const char *branch, char out[MG_HASHSZ]);
void ref_write(const char *branch, const char *commit);
int  ref_delete(const char *branch);
void ref_list(StrList *out);                  /* sorted branch names */
int  ref_valid_name(const char *name);

/* Staging area: a FIFO queue persisted to .minigit/index */
typedef struct IndexEntry {
    char               hash[MG_HASHSZ];
    char               path[MG_PATH_MAX];
    struct IndexEntry *next;
} IndexEntry;

typedef struct {
    IndexEntry *front;
    IndexEntry *rear;
    size_t      count;
} Index;

void index_init(Index *ix);
void index_load(Index *ix);
void index_save(const Index *ix);
void index_enqueue(Index *ix, const char *path, const char *hash);
int  index_remove(Index *ix, const char *path);
const char *index_lookup(const Index *ix, const char *path);
void index_clear(Index *ix);
void index_free(Index *ix);

int  merge_head_read(char out[MG_HASHSZ]);
void merge_head_write(const char *commit);
void merge_head_clear(void);

/* ------------------------------------------------------------------ *
 * object.c - blobs, commits, and the commit DAG
 * ------------------------------------------------------------------ */
typedef struct Commit {
    char           id[MG_HASHSZ];
    char           parent[MG_HASHSZ];  /* "" when root */
    char           parent2[MG_HASHSZ]; /* "" unless a merge commit */
    char           message[MG_MSG_MAX];
    time_t         timestamp;
    struct Commit *next;               /* history list linkage */
} Commit;

int   blob_write(const void *data, size_t len, char out[MG_HASHSZ]);
char *blob_read(const char *hash, size_t *out_len);
int   blob_exists(const char *hash);

int  commit_load(const char *id, Commit *out, HashMap *tree /* may be NULL */);
int  commit_store(const char *parent, const char *parent2, const char *message,
                  time_t ts, const HashMap *tree, char out[MG_HASHSZ]);
int  commit_resolve(const char *ref, char out[MG_HASHSZ]); /* branch | id | unique prefix */
int  commit_exists(const char *id);

Commit *commit_history(const char *id);       /* newest-first linked list; caller frees */
void    commit_free_list(Commit *head);
void    commit_ancestors(const char *id, HashMap *set);
void    commit_depths(const char *id, HashMap *out); /* id -> generation number */
int     commit_is_ancestor(const char *maybe_ancestor, const char *descendant);
int     commit_merge_base(const char *a, const char *b, char out[MG_HASHSZ]);

int  tree_of(const char *commit_id, HashMap *tree);
void tree_checkout(const HashMap *old_tree, const HashMap *new_tree);

/* ------------------------------------------------------------------ *
 * diff.c - longest common subsequence line diff
 * ------------------------------------------------------------------ */
typedef struct {
    int added;
    int removed;
} DiffStat;

DiffStat diff_text(const char *label_a, const char *a,
                   const char *label_b, const char *b, int quiet);

/* ------------------------------------------------------------------ *
 * commands
 * ------------------------------------------------------------------ */
int cmd_init(int argc, char **argv);
int cmd_add(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_status(int argc, char **argv);
int cmd_commit(int argc, char **argv);
int cmd_log(int argc, char **argv);
int cmd_show(int argc, char **argv);
int cmd_branch(int argc, char **argv);
int cmd_checkout(int argc, char **argv);
int cmd_merge(int argc, char **argv);
int cmd_diff(int argc, char **argv);
int cmd_push(int argc, char **argv);
int cmd_pull(int argc, char **argv);

/* 1 when the index is non-empty or a tracked file differs from HEAD. */
int mg_worktree_dirty(void);
/* Switch HEAD to `target` (branch name or commit id), rewriting the tree. */
int mg_switch_to(const char *target, const char *commit_id, int is_branch);

/* Shared colour/formatting helpers used by the command layer. */
extern int mg_use_colour;
const char *C_RED(void);
const char *C_GREEN(void);
const char *C_YELLOW(void);
const char *C_CYAN(void);
const char *C_DIM(void);
const char *C_BOLD(void);
const char *C_OFF(void);

#endif /* MINIGIT_H */
