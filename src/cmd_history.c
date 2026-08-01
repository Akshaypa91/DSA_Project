/*
 * cmd_history.c - commit, log, show
 */
#include "minigit.h"

static void fmt_time(time_t t, char *out, size_t n)
{
    struct tm *tm = localtime(&t);
    if (!tm || strftime(out, n, "%Y-%m-%d %H:%M:%S", tm) == 0)
        snprintf(out, n, "%lld", (long long)t);
}

/* ------------------------------------------------------------------ */

int cmd_commit(int argc, char **argv)
{
    repo_require();

    const char *msg = NULL;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) { msg = argv[++i]; continue; }
        if (!msg) msg = argv[i]; /* positional message also accepted */
    }
    if (!msg || !*msg) {
        fprintf(stderr, "usage: mini_git commit -m \"message\"\n");
        return 1;
    }

    char head_id[MG_HASHSZ] = "";
    int have_head = head_commit(head_id);

    HashMap tree;
    hm_init(&tree);
    if (have_head) tree_of(head_id, &tree);

    Index ix;
    index_load(&ix);

    char merge_id[MG_HASHSZ] = "";
    int merging = merge_head_read(merge_id);

    if (ix.count == 0 && !merging) {
        printf("nothing to commit, working tree clean\n");
        index_free(&ix);
        hm_free(&tree);
        return 1;
    }

    /* Apply the staged queue on top of the parent tree. */
    int changes = 0;
    for (IndexEntry *e = ix.front; e; e = e->next) {
        if (strcmp(e->hash, MG_DELETED) == 0) {
            if (hm_has(&tree, e->path)) { hm_del(&tree, e->path); changes++; }
        } else {
            const char *old = hm_get(&tree, e->path);
            if (!old || strcmp(old, e->hash) != 0) changes++;
            hm_put(&tree, e->path, e->hash);
        }
    }

    if (changes == 0 && !merging) {
        printf("nothing to commit (staged content matches HEAD)\n");
        index_clear(&ix);
        index_free(&ix);
        hm_free(&tree);
        return 1;
    }

    char id[MG_HASHSZ];
    if (commit_store(have_head ? head_id : NULL,
                     merging ? merge_id : NULL,
                     msg, time(NULL), &tree, id) != 0)
        mg_die("cannot write commit object");

    char branch[MG_NAME_MAX];
    if (head_branch(branch, sizeof(branch))) ref_write(branch, id);
    else                                     head_set_detached(id);

    index_clear(&ix);
    index_free(&ix);
    merge_head_clear();

    printf("[%s %s%.8s%s] %s\n",
           head_branch(branch, sizeof(branch)) ? branch : "detached",
           C_YELLOW(), id, C_OFF(), msg);
    printf(" %zu file(s) tracked, %d change(s)\n", tree.count, changes);

    hm_free(&tree);
    return 0;
}

/* ------------------------------------------------------------------ */

int cmd_log(int argc, char **argv)
{
    repo_require();

    long limit = -1;
    int oneline = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--oneline") == 0) oneline = 1;
        else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) limit = strtol(argv[++i], NULL, 10);
    }

    char head_id[MG_HASHSZ];
    if (!head_commit(head_id)) {
        printf("No commits yet.\n");
        return 0;
    }

    char branch[MG_NAME_MAX];
    int on_branch = head_branch(branch, sizeof(branch));

    Commit *hist = commit_history(head_id);
    long shown = 0;
    for (Commit *c = hist; c; c = c->next) {
        if (limit >= 0 && shown >= limit) break;

        if (oneline) {
            printf("%s%.8s%s %s\n", C_YELLOW(), c->id, C_OFF(), c->message);
        } else {
            printf("%scommit %s%s", C_YELLOW(), c->id, C_OFF());
            if (strcmp(c->id, head_id) == 0)
                printf("  %s(HEAD%s%s)%s", C_CYAN(), on_branch ? " -> " : "",
                       on_branch ? branch : "", C_OFF());
            printf("\n");
            if (c->parent2[0]) printf("Merge:  %.8s %.8s\n", c->parent, c->parent2);

            char when[64];
            fmt_time(c->timestamp, when, sizeof(when));
            printf("Date:   %s\n\n    %s\n\n", when, c->message);
        }
        shown++;
    }
    commit_free_list(hist);
    return 0;
}

/* ------------------------------------------------------------------ */

int cmd_show(int argc, char **argv)
{
    repo_require();

    char id[MG_HASHSZ];
    const char *ref = argc > 0 ? argv[0] : "HEAD";
    if (!commit_resolve(ref, id)) {
        mg_warn("unknown revision '%s'", ref);
        return 1;
    }

    Commit c;
    HashMap tree;
    hm_init(&tree);
    if (!commit_load(id, &c, &tree)) {
        hm_free(&tree);
        mg_warn("cannot read commit %s", id);
        return 1;
    }

    char when[64];
    fmt_time(c.timestamp, when, sizeof(when));
    printf("%scommit %s%s\n", C_YELLOW(), c.id, C_OFF());
    if (c.parent[0])  printf("Parent: %.8s\n", c.parent);
    if (c.parent2[0]) printf("Merge:  %.8s\n", c.parent2);
    printf("Date:   %s\n\n    %s\n\n", when, c.message);

    HashMap parent_tree;
    hm_init(&parent_tree);
    if (c.parent[0]) tree_of(c.parent, &parent_tree);

    StrList paths;
    sl_init(&paths);
    hm_keys(&tree, &paths);
    for (size_t i = 0; i < paths.len; i++) {
        const char *now = hm_get(&tree, paths.item[i]);
        const char *was = hm_get(&parent_tree, paths.item[i]);
        const char *tag = !was ? "A" : (strcmp(was, now) == 0 ? " " : "M");
        if (tag[0] != ' ') printf("  %s  %s\n", tag, paths.item[i]);
    }
    sl_free(&paths);

    sl_init(&paths);
    hm_keys(&parent_tree, &paths);
    for (size_t i = 0; i < paths.len; i++)
        if (!hm_has(&tree, paths.item[i])) printf("  D  %s\n", paths.item[i]);
    sl_free(&paths);

    hm_free(&parent_tree);
    hm_free(&tree);
    return 0;
}
