/*
 * cmd_merge.c - three-way merge over the commit DAG.
 *
 * Steps:
 *   1. find the merge base (lowest common ancestor) by BFS over the DAG;
 *   2. if the base is the other head, there is nothing to do;
 *   3. if the base is our head, fast-forward (just move the ref);
 *   4. otherwise compare base/ours/theirs per path:
 *
 *        ours == theirs          -> take either
 *        ours == base            -> take theirs
 *        theirs == base          -> keep ours
 *        all three differ        -> conflict
 *
 * Conflicts are written into the working file with the usual markers and the
 * merge is left in progress (MERGE_HEAD present) until the user resolves,
 * re-adds, and commits.
 */
#include "minigit.h"

static int same(const char *a, const char *b)
{
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    return strcmp(a, b) == 0;
}

static void collect_paths(const HashMap *m, StrList *out)
{
    StrList keys;
    sl_init(&keys);
    hm_keys(m, &keys);
    for (size_t i = 0; i < keys.len; i++)
        if (!sl_has(out, keys.item[i])) sl_push(out, keys.item[i]);
    sl_free(&keys);
}

static void write_conflict(const char *path, const char *ours_hash,
                           const char *theirs_hash, const char *their_name)
{
    size_t ol = 0, tl = 0;
    char *ours   = ours_hash   ? blob_read(ours_hash, &ol)   : NULL;
    char *theirs = theirs_hash ? blob_read(theirs_hash, &tl) : NULL;

    Buffer b;
    buf_init(&b);
    buf_puts(&b, "<<<<<<< ours\n");
    if (ours) buf_append(&b, ours, ol);
    if (ol && ours[ol - 1] != '\n') buf_puts(&b, "\n");
    buf_puts(&b, "=======\n");
    if (theirs) buf_append(&b, theirs, tl);
    if (tl && theirs[tl - 1] != '\n') buf_puts(&b, "\n");
    buf_printf(&b, ">>>>>>> %s\n", their_name);

    mg_write_file(path, b.data, b.len);
    buf_free(&b);
    free(ours);
    free(theirs);
}

int cmd_merge(int argc, char **argv)
{
    repo_require();

    if (argc < 1) {
        fprintf(stderr, "usage: mini_git merge <branch|commit>\n");
        return 1;
    }
    const char *their_name = argv[0];

    char ours[MG_HASHSZ];
    if (!head_commit(ours)) {
        mg_warn("nothing to merge into: no commits yet");
        return 1;
    }

    char theirs[MG_HASHSZ];
    if (!commit_resolve(their_name, theirs)) {
        mg_warn("no branch or commit named '%s'", their_name);
        return 1;
    }

    if (strcmp(ours, theirs) == 0) {
        printf("Already up to date.\n");
        return 0;
    }

    if (mg_worktree_dirty()) {
        mg_warn("you have local changes; commit them before merging");
        return 1;
    }

    char base[MG_HASHSZ] = "";
    int have_base = commit_merge_base(ours, theirs, base);

    if (have_base && strcmp(base, theirs) == 0) {
        printf("Already up to date.\n");
        return 0;
    }

    char branch[MG_NAME_MAX];
    int on_branch = head_branch(branch, sizeof(branch));

    /* ---- fast-forward ---- */
    if (have_base && strcmp(base, ours) == 0) {
        HashMap old_tree, new_tree;
        hm_init(&old_tree);
        hm_init(&new_tree);
        tree_of(ours, &old_tree);
        tree_of(theirs, &new_tree);
        tree_checkout(&old_tree, &new_tree);
        hm_free(&old_tree);
        hm_free(&new_tree);

        if (on_branch) ref_write(branch, theirs);
        else           head_set_detached(theirs);

        printf("Fast-forward %.8s..%.8s\n", ours, theirs);
        return 0;
    }

    /* ---- true three-way merge ---- */
    HashMap tb, to, tt;
    hm_init(&tb);
    hm_init(&to);
    hm_init(&tt);
    if (have_base) tree_of(base, &tb);
    tree_of(ours, &to);
    tree_of(theirs, &tt);

    StrList paths;
    sl_init(&paths);
    collect_paths(&tb, &paths);
    collect_paths(&to, &paths);
    collect_paths(&tt, &paths);
    sl_sort(&paths);

    HashMap merged;
    hm_init(&merged);
    StrList conflicts;
    sl_init(&conflicts);
    int changed = 0;

    for (size_t i = 0; i < paths.len; i++) {
        const char *p = paths.item[i];
        const char *b = hm_get(&tb, p);
        const char *o = hm_get(&to, p);
        const char *t = hm_get(&tt, p);

        if (same(o, t)) {
            if (o) hm_put(&merged, p, o);
        } else if (same(o, b)) {                 /* only they changed it */
            if (t) { hm_put(&merged, p, t); changed++; }
            else   { changed++; }                /* they deleted it */
        } else if (same(t, b)) {                 /* only we changed it */
            if (o) hm_put(&merged, p, o);
        } else if (o && t) {                     /* both changed: conflict */
            write_conflict(p, o, t, their_name);
            char h[MG_HASHSZ];
            if (mg_hash_path(p, h) == 0) {
                size_t len;
                char *data = mg_read_file(p, &len);
                if (data) { blob_write(data, len, h); free(data); }
                hm_put(&merged, p, h);
            }
            sl_push(&conflicts, p);
        } else {                                  /* modify/delete conflict */
            if (o) hm_put(&merged, p, o);
            sl_push(&conflicts, p);
            mg_warn("modify/delete conflict on '%s' (kept our version)", p);
        }
    }

    tree_checkout(&to, &merged);

    if (conflicts.len > 0) {
        Index ix;
        index_init(&ix);
        StrList mp;
        sl_init(&mp);
        hm_keys(&merged, &mp);
        for (size_t i = 0; i < mp.len; i++)
            if (!sl_has(&conflicts, mp.item[i]))
                index_enqueue(&ix, mp.item[i], hm_get(&merged, mp.item[i]));
        index_save(&ix);
        index_free(&ix);
        sl_free(&mp);

        merge_head_write(theirs);

        printf("%sAutomatic merge failed; %zu conflict(s):%s\n",
               C_RED(), conflicts.len, C_OFF());
        for (size_t i = 0; i < conflicts.len; i++)
            printf("  %s\n", conflicts.item[i]);
        printf("Fix the conflicts, then run: mini_git add <file> && "
               "mini_git commit -m \"merge\"\n");
    } else {
        char msg[MG_MSG_MAX];
        snprintf(msg, sizeof(msg), "Merge '%s' into %s",
                 their_name, on_branch ? branch : "detached HEAD");

        char id[MG_HASHSZ];
        if (commit_store(ours, theirs, msg, time(NULL), &merged, id) != 0)
            mg_die("cannot write merge commit");

        if (on_branch) ref_write(branch, id);
        else           head_set_detached(id);

        Index ix;
        index_init(&ix);
        index_save(&ix);
        merge_head_clear();

        printf("Merge made by the three-way strategy: %s%.8s%s "
               "(%d file(s) taken from '%s')\n",
               C_YELLOW(), id, C_OFF(), changed, their_name);
    }

    int rc = conflicts.len > 0 ? 1 : 0;
    sl_free(&conflicts);
    sl_free(&paths);
    hm_free(&merged);
    hm_free(&tb);
    hm_free(&to);
    hm_free(&tt);
    return rc;
}
