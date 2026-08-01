/*
 * diff.c - line-oriented diff via longest common subsequence.
 *
 * Classic dynamic programming.  For inputs of n and m lines the table is
 * (n+1) x (m+1); filling it is O(n*m) time and O(n*m) space, and the edit
 * script is recovered by walking the table backwards in O(n+m).
 *
 * Two deliberate changes from the naive textbook version:
 *   - the table is one contiguous heap allocation instead of a 2-D stack
 *     array, so large files do not blow the stack;
 *   - the backtrack is iterative rather than recursive, for the same reason.
 *
 * Output is shown with three lines of context; unchanged runs longer than
 * that collapse into a "@@" marker.
 */
#include "minigit.h"

#define DIFF_CONTEXT     3
#define DIFF_MAX_CELLS   40000000ULL /* ~160 MB of int; refuse beyond this */

typedef struct {
    char  **line;
    size_t  count;
    char   *store; /* owns the split copy */
} Lines;

static void lines_split(const char *text, Lines *out)
{
    out->line = NULL;
    out->count = 0;
    out->store = NULL;
    if (!text) return;

    out->store = mg_strdup(text);

    size_t cap = 32;
    out->line = mg_malloc(cap * sizeof(*out->line));

    char *p = out->store;
    while (*p) {
        if (out->count == cap) {
            cap *= 2;
            out->line = mg_realloc(out->line, cap * sizeof(*out->line));
        }
        out->line[out->count++] = p;

        char *nl = strchr(p, '\n');
        if (!nl) break;
        *nl = '\0';
        if (nl > p && nl[-1] == '\r') nl[-1] = '\0';
        p = nl + 1;
    }
}

static void lines_free(Lines *l)
{
    free(l->line);
    free(l->store);
    l->line = NULL;
    l->store = NULL;
    l->count = 0;
}

typedef struct {
    char  tag;   /* ' ', '+', '-' */
    const char *text;
} Op;

DiffStat diff_text(const char *label_a, const char *a,
                   const char *label_b, const char *b, int quiet)
{
    DiffStat st = { 0, 0 };

    Lines A, B;
    lines_split(a, &A);
    lines_split(b, &B);

    size_t n = A.count, m = B.count;

    if ((unsigned long long)(n + 1) * (m + 1) > DIFF_MAX_CELLS) {
        if (!quiet)
            printf("%s and %s are too large to diff line by line "
                   "(%zu vs %zu lines)\n", label_a, label_b, n, m);
        st.added = (int)m;
        st.removed = (int)n;
        lines_free(&A);
        lines_free(&B);
        return st;
    }

    size_t cols = m + 1;
    int *dp = mg_calloc((n + 1) * cols, sizeof(int));

    for (size_t i = 1; i <= n; i++) {
        for (size_t j = 1; j <= m; j++) {
            if (strcmp(A.line[i - 1], B.line[j - 1]) == 0)
                dp[i * cols + j] = dp[(i - 1) * cols + (j - 1)] + 1;
            else {
                int up   = dp[(i - 1) * cols + j];
                int left = dp[i * cols + (j - 1)];
                dp[i * cols + j] = up > left ? up : left;
            }
        }
    }

    /* Backtrack into a reversed op list. */
    size_t cap = n + m + 1;
    Op *ops = mg_malloc(cap * sizeof(*ops));
    size_t nops = 0;

    size_t i = n, j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && strcmp(A.line[i - 1], B.line[j - 1]) == 0) {
            ops[nops].tag = ' ';
            ops[nops++].text = A.line[i - 1];
            i--; j--;
        } else if (j > 0 && (i == 0 || dp[i * cols + (j - 1)] >= dp[(i - 1) * cols + j])) {
            ops[nops].tag = '+';
            ops[nops++].text = B.line[j - 1];
            j--;
            st.added++;
        } else {
            ops[nops].tag = '-';
            ops[nops++].text = A.line[i - 1];
            i--;
            st.removed++;
        }
    }
    free(dp);

    /* Reverse into chronological order. */
    for (size_t x = 0, y = nops ? nops - 1 : 0; x < y; x++, y--) {
        Op t = ops[x]; ops[x] = ops[y]; ops[y] = t;
    }

    if (!quiet && nops > 0) {
        printf("%s--- %s%s\n", C_BOLD(), label_a, C_OFF());
        printf("%s+++ %s%s\n", C_BOLD(), label_b, C_OFF());

        /* Mark which context lines to keep. */
        char *keep = mg_calloc(nops ? nops : 1, 1);
        for (size_t k = 0; k < nops; k++) {
            if (ops[k].tag == ' ') continue;
            size_t lo = k > DIFF_CONTEXT ? k - DIFF_CONTEXT : 0;
            size_t hi = k + DIFF_CONTEXT + 1;
            if (hi > nops) hi = nops;
            for (size_t q = lo; q < hi; q++) keep[q] = 1;
        }

        int skipping = 0;
        for (size_t k = 0; k < nops; k++) {
            if (!keep[k]) {
                if (!skipping) { printf("%s@@ ...%s\n", C_CYAN(), C_OFF()); skipping = 1; }
                continue;
            }
            skipping = 0;
            if (ops[k].tag == '+')
                printf("%s+ %s%s\n", C_GREEN(), ops[k].text, C_OFF());
            else if (ops[k].tag == '-')
                printf("%s- %s%s\n", C_RED(), ops[k].text, C_OFF());
            else
                printf("  %s\n", ops[k].text);
        }
        free(keep);
        printf("%s%d insertion(s), %d deletion(s)%s\n",
               C_DIM(), st.added, st.removed, C_OFF());
    }

    free(ops);
    lines_free(&A);
    lines_free(&B);
    return st;
}
