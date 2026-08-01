/*
 * repo.c - repository layout, HEAD, branch refs, and the staging index.
 *
 * On-disk layout:
 *
 *   .minigit/
 *     HEAD              "ref: <branch>"  or  "commit: <id>" when detached
 *     MERGE_HEAD        present only while a conflicted merge is unresolved
 *     index             staging queue, one "<hash> <path>" line per entry
 *     refs/heads/<name> a single commit id
 *     objects/blobs/xx/ content-addressed file snapshots (deduplicated)
 *     objects/commits/  commit metadata objects
 *
 * The staging area is a FIFO queue: `add` enqueues, `commit` drains it in
 * insertion order.  Re-adding a path updates the existing node in place so
 * a file can never appear twice in the index.
 */
#include "minigit.h"

#include <stdarg.h>

/* ------------------------------------------------------------------ */

int repo_exists(void)
{
    return mg_is_dir(MG_DIR);
}

void repo_require(void)
{
    if (!repo_exists())
        mg_die("not a MiniGit repository (run '%s init' first)", "mini_git");
}

void repo_path(char *out, size_t n, const char *fmt, ...)
{
    char tail[MG_PATH_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tail, sizeof(tail), fmt, ap);
    va_end(ap);
    snprintf(out, n, "%s/%s", MG_DIR, tail);
}

/* ------------------------------------------------------------------ *
 * HEAD
 * ------------------------------------------------------------------ */

int head_branch(char *out, size_t n)
{
    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "HEAD");

    char *txt = mg_read_file(p, NULL);
    if (!txt) return 0;

    char *s = mg_trim(txt);
    int ok = 0;
    if (strncmp(s, "ref:", 4) == 0) {
        snprintf(out, n, "%s", mg_trim(s + 4));
        ok = out[0] != '\0';
    }
    free(txt);
    return ok;
}

int head_commit(char out[MG_HASHSZ])
{
    char branch[MG_NAME_MAX];
    if (head_branch(branch, sizeof(branch)))
        return ref_read(branch, out);

    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "HEAD");
    char *txt = mg_read_file(p, NULL);
    if (!txt) return 0;

    char *s = mg_trim(txt);
    int ok = 0;
    if (strncmp(s, "commit:", 7) == 0) {
        snprintf(out, MG_HASHSZ, "%s", mg_trim(s + 7));
        ok = out[0] != '\0';
    }
    free(txt);
    return ok;
}

void head_set_branch(const char *branch)
{
    char p[MG_PATH_MAX], line[MG_PATH_MAX];
    repo_path(p, sizeof(p), "HEAD");
    int n = snprintf(line, sizeof(line), "ref: %s\n", branch);
    if (mg_write_file(p, line, (size_t)n) != 0)
        mg_die("cannot update HEAD");
}

void head_set_detached(const char *commit)
{
    char p[MG_PATH_MAX], line[MG_PATH_MAX];
    repo_path(p, sizeof(p), "HEAD");
    int n = snprintf(line, sizeof(line), "commit: %s\n", commit);
    if (mg_write_file(p, line, (size_t)n) != 0)
        mg_die("cannot update HEAD");
}

void head_describe(char *out, size_t n)
{
    char branch[MG_NAME_MAX], id[MG_HASHSZ];
    if (head_branch(branch, sizeof(branch)))
        snprintf(out, n, "%s", branch);
    else if (head_commit(id))
        snprintf(out, n, "HEAD detached at %.8s", id);
    else
        snprintf(out, n, "unknown");
}

/* ------------------------------------------------------------------ *
 * Branch refs
 * ------------------------------------------------------------------ */

int ref_valid_name(const char *name)
{
    if (!name || !*name) return 0;
    if (strlen(name) >= MG_NAME_MAX) return 0;
    if (name[0] == '-' || name[0] == '.') return 0;
    for (const char *p = name; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ' ' || *p == '\t' ||
            *p == ':' || *p == '?' || *p == '*' || *p == '~' || *p == '^')
            return 0;
    }
    return 1;
}

int ref_read(const char *branch, char out[MG_HASHSZ])
{
    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "refs/heads/%s", branch);
    char *txt = mg_read_file(p, NULL);
    if (!txt) return 0;
    snprintf(out, MG_HASHSZ, "%s", mg_trim(txt));
    free(txt);
    return out[0] != '\0';
}

void ref_write(const char *branch, const char *commit)
{
    char p[MG_PATH_MAX], line[MG_HASHSZ + 2];
    repo_path(p, sizeof(p), "refs/heads/%s", branch);
    int n = snprintf(line, sizeof(line), "%s\n", commit);
    if (mg_write_file(p, line, (size_t)n) != 0)
        mg_die("cannot write ref '%s'", branch);
}

int ref_delete(const char *branch)
{
    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "refs/heads/%s", branch);
    if (!mg_exists(p)) return -1;
    return mg_unlink(p);
}

void ref_list(StrList *out)
{
    char dir[MG_PATH_MAX];
    repo_path(dir, sizeof(dir), "refs/heads");
    mg_list_dir(dir, out);
}

/* ------------------------------------------------------------------ *
 * Staging index (FIFO queue)
 * ------------------------------------------------------------------ */

void index_init(Index *ix)
{
    ix->front = ix->rear = NULL;
    ix->count = 0;
}

void index_load(Index *ix)
{
    index_init(ix);

    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "index");
    char *txt = mg_read_file(p, NULL);
    if (!txt) return;

    char *save = txt, *line = txt;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        char *t = mg_trim(line);
        if (*t) {
            char *sp = strchr(t, ' ');
            if (sp) {
                *sp = '\0';
                index_enqueue(ix, mg_trim(sp + 1), t);
            }
        }
        line = nl ? nl + 1 : NULL;
    }
    free(save);
}

void index_save(const Index *ix)
{
    Buffer b;
    buf_init(&b);
    for (IndexEntry *e = ix->front; e; e = e->next)
        buf_printf(&b, "%s %s\n", e->hash, e->path);

    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "index");
    if (mg_write_file(p, b.data, b.len) != 0)
        mg_die("cannot write index");
    buf_free(&b);
}

void index_enqueue(Index *ix, const char *path, const char *hash)
{
    for (IndexEntry *e = ix->front; e; e = e->next) {
        if (strcmp(e->path, path) == 0) { /* update in place, keep position */
            snprintf(e->hash, MG_HASHSZ, "%s", hash);
            return;
        }
    }

    IndexEntry *e = mg_malloc(sizeof(*e));
    snprintf(e->hash, MG_HASHSZ, "%s", hash);
    snprintf(e->path, MG_PATH_MAX, "%s", path);
    e->next = NULL;

    if (ix->rear) ix->rear->next = e;
    else          ix->front = e;
    ix->rear = e;
    ix->count++;
}

int index_remove(Index *ix, const char *path)
{
    IndexEntry *prev = NULL, *e = ix->front;
    while (e) {
        if (strcmp(e->path, path) == 0) {
            if (prev) prev->next = e->next;
            else      ix->front  = e->next;
            if (ix->rear == e) ix->rear = prev;
            free(e);
            ix->count--;
            return 1;
        }
        prev = e;
        e = e->next;
    }
    return 0;
}

const char *index_lookup(const Index *ix, const char *path)
{
    for (IndexEntry *e = ix->front; e; e = e->next)
        if (strcmp(e->path, path) == 0) return e->hash;
    return NULL;
}

void index_clear(Index *ix)
{
    index_free(ix);
    index_save(ix);
}

void index_free(Index *ix)
{
    IndexEntry *e = ix->front;
    while (e) {
        IndexEntry *next = e->next;
        free(e);
        e = next;
    }
    index_init(ix);
}

/* ------------------------------------------------------------------ *
 * MERGE_HEAD - records the other parent of an in-progress merge
 * ------------------------------------------------------------------ */

int merge_head_read(char out[MG_HASHSZ])
{
    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "MERGE_HEAD");
    char *txt = mg_read_file(p, NULL);
    if (!txt) return 0;
    snprintf(out, MG_HASHSZ, "%s", mg_trim(txt));
    free(txt);
    return out[0] != '\0';
}

void merge_head_write(const char *commit)
{
    char p[MG_PATH_MAX], line[MG_HASHSZ + 2];
    repo_path(p, sizeof(p), "MERGE_HEAD");
    int n = snprintf(line, sizeof(line), "%s\n", commit);
    mg_write_file(p, line, (size_t)n);
}

void merge_head_clear(void)
{
    char p[MG_PATH_MAX];
    repo_path(p, sizeof(p), "MERGE_HEAD");
    if (mg_exists(p)) mg_unlink(p);
}
