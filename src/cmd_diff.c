/*
 * cmd_diff.c - the `diff` command.
 *
 *   mini_git diff                    working tree vs HEAD
 *   mini_git diff <commit>           working tree vs that commit
 *   mini_git diff <commit> <commit>  tree vs tree
 *   mini_git diff <fileA> <fileB>    two files on disk
 */
#include "minigit.h"

static void diff_trees(const HashMap *a, const HashMap *b,
                       const char *label_a, const char *label_b,
                       int b_is_worktree)
{
    StrList paths, extra;
    sl_init(&paths);
    sl_init(&extra);
    hm_keys(a, &paths);
    hm_keys(b, &extra);
    for (size_t i = 0; i < extra.len; i++)
        if (!sl_has(&paths, extra.item[i])) sl_push(&paths, extra.item[i]);
    sl_free(&extra);
    sl_sort(&paths);

    int any = 0;
    for (size_t i = 0; i < paths.len; i++) {
        const char *p  = paths.item[i];
        const char *ha = hm_get(a, p);
        const char *hb = hm_get(b, p);
        if (ha && hb && strcmp(ha, hb) == 0) continue;

        char la[MG_PATH_MAX + 32], lb[MG_PATH_MAX + 32];
        snprintf(la, sizeof(la), "%s:%s", label_a, ha ? p : "(absent)");
        snprintf(lb, sizeof(lb), "%s:%s", label_b, hb ? p : "(absent)");

        char *ca = ha ? blob_read(ha, NULL) : NULL;
        char *cb = NULL;
        if (hb) cb = b_is_worktree ? mg_read_file(p, NULL) : blob_read(hb, NULL);

        diff_text(la, ca ? ca : "", lb, cb ? cb : "", 0);
        printf("\n");
        free(ca);
        free(cb);
        any = 1;
    }
    if (!any) printf("No differences.\n");
    sl_free(&paths);
}

/* Hash the on-disk state of every path tracked by `head_tree`. */
static void worktree_tree(const HashMap *head_tree, HashMap *out)
{
    StrList paths;
    sl_init(&paths);
    hm_keys(head_tree, &paths);
    for (size_t i = 0; i < paths.len; i++) {
        char h[MG_HASHSZ];
        if (mg_hash_path(paths.item[i], h) == 0) hm_put(out, paths.item[i], h);
    }
    sl_free(&paths);
}

static int looks_like_file(const char *s)
{
    return mg_exists(s) && !mg_is_dir(s);
}

int cmd_diff(int argc, char **argv)
{
    repo_require();

    /*
     * Disambiguation rule: when both arguments name existing files, they are
     * treated as paths.  Revisions are only considered otherwise.  Without
     * this, a filename that happens to be a hex prefix of some commit id
     * (say "f1") would silently resolve as a revision, and which behaviour
     * you got would depend on the hashes in the repository.
     */
    if (argc == 2 && looks_like_file(argv[0]) && looks_like_file(argv[1])) {
        char *a = mg_read_file(argv[0], NULL);
        char *b = mg_read_file(argv[1], NULL);
        if (!a || !b) {
            mg_warn("cannot read input files");
            free(a);
            free(b);
            return 1;
        }
        diff_text(argv[0], a, argv[1], b, 0);
        free(a);
        free(b);
        return 0;
    }

    if (argc >= 2) {
        char ida[MG_HASHSZ], idb[MG_HASHSZ];
        if (!commit_resolve(argv[0], ida)) { mg_warn("unknown revision '%s'", argv[0]); return 1; }
        if (!commit_resolve(argv[1], idb)) { mg_warn("unknown revision '%s'", argv[1]); return 1; }

        HashMap ta, tb;
        hm_init(&ta);
        hm_init(&tb);
        tree_of(ida, &ta);
        tree_of(idb, &tb);

        char la[16], lb[16];
        snprintf(la, sizeof(la), "%.8s", ida);
        snprintf(lb, sizeof(lb), "%.8s", idb);
        diff_trees(&ta, &tb, la, lb, 0);

        hm_free(&ta);
        hm_free(&tb);
        return 0;
    }

    char id[MG_HASHSZ];
    const char *ref = argc == 1 ? argv[0] : "HEAD";
    if (!commit_resolve(ref, id)) {
        if (argc == 1) { mg_warn("unknown revision '%s'", ref); return 1; }
        printf("No commits yet.\n");
        return 0;
    }

    HashMap tree, work;
    hm_init(&tree);
    hm_init(&work);
    tree_of(id, &tree);
    worktree_tree(&tree, &work);

    char la[16];
    snprintf(la, sizeof(la), "%.8s", id);
    diff_trees(&tree, &work, la, "worktree", 1);

    hm_free(&tree);
    hm_free(&work);
    return 0;
}
