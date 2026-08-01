/*
 * util.c - allocation, hashing, growable containers, filesystem helpers.
 *
 * This is the only translation unit that touches POSIX APIs directly
 * (opendir, isatty, mkdir), so the feature-test macro lives here and the
 * rest of the project stays plain C11.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200809L
#endif

#include "minigit.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#  define MG_MKDIR_ONE(p) _mkdir(p)
#else
#  include <unistd.h>
#  define MG_MKDIR_ONE(p) mkdir((p), 0755)
#endif

#include <dirent.h>

/* ------------------------------------------------------------------ *
 * Diagnostics and allocation
 * ------------------------------------------------------------------ */
void mg_die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("minigit: fatal: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

void mg_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("minigit: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void *mg_malloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) mg_die("out of memory (%zu bytes)", n);
    return p;
}

void *mg_calloc(size_t n, size_t sz)
{
    void *p = calloc(n ? n : 1, sz ? sz : 1);
    if (!p) mg_die("out of memory (%zu x %zu bytes)", n, sz);
    return p;
}

void *mg_realloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) mg_die("out of memory (%zu bytes)", n);
    return q;
}

char *mg_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char  *p = mg_malloc(n);
    memcpy(p, s, n);
    return p;
}

/* ------------------------------------------------------------------ *
 * FNV-1a 64-bit
 *
 * Chosen over the original `hash*31 + c` because it mixes every byte of
 * binary content, has good avalanche behaviour, and is trivial to
 * implement correctly.  Object ids are the low 64 bits in hex.
 * ------------------------------------------------------------------ */
#define FNV_OFFSET 1469598103934665603ULL
#define FNV_PRIME  1099511628211ULL

uint64_t mg_fnv1a(const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= FNV_PRIME;
    }
    return h;
}

uint64_t mg_fnv1a_str(const char *s)
{
    return mg_fnv1a(s, strlen(s));
}

void mg_hash_hex(uint64_t h, char out[MG_HASHSZ])
{
    static const char hex[] = "0123456789abcdef";
    for (int i = MG_HASHLEN - 1; i >= 0; i--) {
        out[i] = hex[h & 0xF];
        h >>= 4;
    }
    out[MG_HASHLEN] = '\0';
}

void mg_hash_data(const void *data, size_t len, char out[MG_HASHSZ])
{
    /* Length is folded in so that "ab"+"c" and "a"+"bc" cannot collide
     * when the hash is used over concatenated fields. */
    uint64_t h = mg_fnv1a(data, len);
    h ^= (uint64_t)len * FNV_PRIME;
    mg_hash_hex(h, out);
}

/* ------------------------------------------------------------------ *
 * Buffer - amortised O(1) append via capacity doubling
 * ------------------------------------------------------------------ */
void buf_init(Buffer *b)
{
    b->data = mg_malloc(64);
    b->data[0] = '\0';
    b->len = 0;
    b->cap = 64;
}

static void buf_reserve(Buffer *b, size_t need)
{
    if (b->len + need + 1 <= b->cap) return;
    size_t cap = b->cap ? b->cap : 64;
    while (cap < b->len + need + 1) cap *= 2;
    b->data = mg_realloc(b->data, cap);
    b->cap  = cap;
}

void buf_append(Buffer *b, const void *s, size_t n)
{
    buf_reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

void buf_puts(Buffer *b, const char *s)
{
    buf_append(b, s, strlen(s));
}

void buf_printf(Buffer *b, const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) { va_end(ap2); return; }
    buf_reserve(b, (size_t)n);
    vsnprintf(b->data + b->len, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    b->len += (size_t)n;
}

void buf_free(Buffer *b)
{
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* ------------------------------------------------------------------ *
 * StrList
 * ------------------------------------------------------------------ */
void sl_init(StrList *l)
{
    l->item = NULL;
    l->len = l->cap = 0;
}

void sl_push(StrList *l, const char *s)
{
    if (l->len == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->item = mg_realloc(l->item, l->cap * sizeof(*l->item));
    }
    l->item[l->len++] = mg_strdup(s);
}

int sl_has(const StrList *l, const char *s)
{
    for (size_t i = 0; i < l->len; i++)
        if (strcmp(l->item[i], s) == 0) return 1;
    return 0;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

void sl_sort(StrList *l)
{
    if (l->len > 1) qsort(l->item, l->len, sizeof(*l->item), cmp_str);
}

void sl_free(StrList *l)
{
    for (size_t i = 0; i < l->len; i++) free(l->item[i]);
    free(l->item);
    l->item = NULL;
    l->len = l->cap = 0;
}

/* ------------------------------------------------------------------ *
 * Filesystem
 * ------------------------------------------------------------------ */
int mg_mkdir(const char *path)
{
    if (MG_MKDIR_ONE(path) == 0) return 0;
    return (errno == EEXIST) ? 0 : -1;
}

int mg_mkdir_p(const char *path)
{
    char tmp[MG_PATH_MAX];
    size_t n = strlen(path);
    if (n == 0 || n >= sizeof(tmp)) return -1;
    memcpy(tmp, path, n + 1);

    for (size_t i = 1; i < n; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char c = tmp[i];
            tmp[i] = '\0';
            if (tmp[0] && mg_mkdir(tmp) != 0) return -1;
            tmp[i] = c;
        }
    }
    return mg_mkdir(tmp);
}

int mg_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

int mg_is_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

char *mg_read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);

    char *buf = mg_malloc((size_t)size + 1);
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[got] = '\0';
    if (out_len) *out_len = got;
    return buf;
}

int mg_write_file(const char *path, const void *data, size_t len)
{
    char dir[MG_PATH_MAX];
    mg_parent_dir(path, dir, sizeof(dir));
    if (dir[0] && !mg_exists(dir) && mg_mkdir_p(dir) != 0) return -1;

    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t wrote = len ? fwrite(data, 1, len, f) : 0;
    int ok = (wrote == len);
    if (fclose(f) != 0) ok = 0;
    return ok ? 0 : -1;
}

int mg_copy_file(const char *src, const char *dst)
{
    size_t len;
    char *data = mg_read_file(src, &len);
    if (!data) return -1;
    int rc = mg_write_file(dst, data, len);
    free(data);
    return rc;
}

int mg_copy_tree(const char *src, const char *dst)
{
    if (!mg_is_dir(src)) return mg_copy_file(src, dst);
    if (mg_mkdir_p(dst) != 0) return -1;

    StrList kids;
    sl_init(&kids);
    if (mg_list_dir(src, &kids) != 0) { sl_free(&kids); return -1; }

    int rc = 0;
    for (size_t i = 0; i < kids.len && rc == 0; i++) {
        char s[MG_PATH_MAX], d[MG_PATH_MAX];
        snprintf(s, sizeof(s), "%s/%s", src, kids.item[i]);
        snprintf(d, sizeof(d), "%s/%s", dst, kids.item[i]);
        rc = mg_copy_tree(s, d);
    }
    sl_free(&kids);
    return rc;
}

int mg_unlink(const char *path)
{
    return remove(path);
}

void mg_parent_dir(const char *path, char *out, size_t n)
{
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *bs = strrchr(path, '\\');
    if (bs && (!slash || bs > slash)) slash = bs;
#endif
    if (!slash) { out[0] = '\0'; return; }
    size_t len = (size_t)(slash - path);
    if (len >= n) len = n - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

char *mg_trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    return s;
}

int mg_list_dir(const char *path, StrList *out)
{
    DIR *d = opendir(path);
    if (!d) return -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        sl_push(out, e->d_name);
    }
    closedir(d);
    sl_sort(out);
    return 0;
}

/* ---------------- ignore rules ---------------- */

static StrList ignore_rules;
static int     ignore_loaded = 0;

static void ignore_load(void)
{
    if (ignore_loaded) return;
    ignore_loaded = 1;
    sl_init(&ignore_rules);

    size_t len;
    char *txt = mg_read_file(".minigitignore", &len);
    if (!txt) return;

    char *save = txt;
    char *line = txt;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        char *t = mg_trim(line);
        if (*t && *t != '#') sl_push(&ignore_rules, t);
        line = nl ? nl + 1 : NULL;
    }
    free(save);
}

static int ignore_match(const char *path, const char *name)
{
    /* Always-ignored: VCS metadata, build output, editor cruft. */
    static const char *always[] = { MG_DIR, ".git", ".minigit", "build",
                                    "mini_git", "mini_git.exe", ".cph", NULL };
    for (int i = 0; always[i]; i++)
        if (strcmp(name, always[i]) == 0) return 1;
    if (name[0] == '.') return 1;

    ignore_load();
    for (size_t i = 0; i < ignore_rules.len; i++) {
        const char *r = ignore_rules.item[i];
        size_t rl = strlen(r);
        if (strcmp(r, path) == 0 || strcmp(r, name) == 0) return 1;
        if (rl > 0 && r[rl - 1] == '/' && strncmp(path, r, rl) == 0) return 1;
        if (strncmp(path, r, rl) == 0 && path[rl] == '/') return 1;
        if (r[0] == '*' && rl > 1) {
            size_t pl = strlen(path);
            if (pl >= rl - 1 && strcmp(path + pl - (rl - 1), r + 1) == 0) return 1;
        }
    }
    return 0;
}

static void walk(const char *dir, const char *prefix, StrList *out)
{
    StrList kids;
    sl_init(&kids);
    if (mg_list_dir(dir, &kids) != 0) { sl_free(&kids); return; }

    for (size_t i = 0; i < kids.len; i++) {
        const char *name = kids.item[i];
        char rel[MG_PATH_MAX], full[MG_PATH_MAX];
        if (prefix[0])
            snprintf(rel, sizeof(rel), "%s/%s", prefix, name);
        else
            snprintf(rel, sizeof(rel), "%s", name);
        snprintf(full, sizeof(full), "%s/%s", dir, name);

        if (ignore_match(rel, name)) continue;

        if (mg_is_dir(full)) walk(full, rel, out);
        else                 sl_push(out, rel);
    }
    sl_free(&kids);
}

void mg_walk_worktree(StrList *out)
{
    walk(".", "", out);
    sl_sort(out);
}

int mg_hash_path(const char *path, char out[MG_HASHSZ])
{
    size_t len;
    char *data = mg_read_file(path, &len);
    if (!data) return -1;
    mg_hash_data(data, len, out);
    free(data);
    return 0;
}

/* ------------------------------------------------------------------ *
 * Colour output (disabled when not a TTY or when NO_COLOR is set)
 * ------------------------------------------------------------------ */
int mg_use_colour = -1; /* lazily determined */

static int colour_on(void)
{
    if (mg_use_colour < 0) {
#ifdef _WIN32
        mg_use_colour = 0;
#else
        mg_use_colour = (getenv("NO_COLOR") == NULL) && isatty(1);
#endif
    }
    return mg_use_colour;
}

const char *C_RED(void)    { return colour_on() ? "\033[31m" : ""; }
const char *C_GREEN(void)  { return colour_on() ? "\033[32m" : ""; }
const char *C_YELLOW(void) { return colour_on() ? "\033[33m" : ""; }
const char *C_CYAN(void)   { return colour_on() ? "\033[36m" : ""; }
const char *C_DIM(void)    { return colour_on() ? "\033[2m"  : ""; }
const char *C_BOLD(void)   { return colour_on() ? "\033[1m"  : ""; }
const char *C_OFF(void)    { return colour_on() ? "\033[0m"  : ""; }
