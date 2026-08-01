/*
 * cmd_branch.c - branch, checkout, and the shared switch/dirty helpers.
 *
 * Branches are just named pointers into the commit DAG, so creating one is
 * O(1): write the current commit id into refs/heads/<name>.
 */
#include "minigit.h"

int mg_worktree_dirty(void)
{
    Index ix;
    index_load(&ix);
    int dirty = ix.count > 0;
    index_free(&ix);
    if (dirty) return 1;

    char head_id[MG_HASHSZ];
    if (!head_commit(head_id)) return 0;

    HashMap tree;
    hm_init(&tree);
    tree_of(head_id, &tree);

    StrList paths;
    sl_init(&paths);
    hm_keys(&tree, &paths);
    for (size_t i = 0; i < paths.len && !dirty; i++) {
        char now[MG_HASHSZ];
        if (!mg_exists(paths.item[i])) dirty = 1;
        else if (mg_hash_path(paths.item[i], now) == 0 &&
                 strcmp(now, hm_get(&tree, paths.item[i])) != 0) dirty = 1;
    }
    sl_free(&paths);
    hm_free(&tree);
    return dirty;
}

int mg_switch_to(const char *target, const char *commit_id, int is_branch)
{
    HashMap old_tree, new_tree;
    hm_init(&old_tree);
    hm_init(&new_tree);

    char cur[MG_HASHSZ];
    if (head_commit(cur)) tree_of(cur, &old_tree);
    if (commit_id && *commit_id) tree_of(commit_id, &new_tree);

    tree_checkout(&old_tree, &new_tree);

    if (is_branch) head_set_branch(target);
    else           head_set_detached(commit_id);

    Index ix;
    index_init(&ix);
    index_save(&ix);
    merge_head_clear();

    hm_free(&old_tree);
    hm_free(&new_tree);
    return 0;
}

/* ------------------------------------------------------------------ */

static int branch_list(void)
{
    StrList branches;
    sl_init(&branches);
    ref_list(&branches);

    if (branches.len == 0) {
        printf("No branches yet -- make a commit first.\n");
        sl_free(&branches);
        return 0;
    }

    char cur[MG_NAME_MAX] = "";
    int on_branch = head_branch(cur, sizeof(cur));

    for (size_t i = 0; i < branches.len; i++) {
        char id[MG_HASHSZ] = "";
        ref_read(branches.item[i], id);
        int is_cur = on_branch && strcmp(cur, branches.item[i]) == 0;
        printf("%s%s %-20s %s%.8s%s\n",
               is_cur ? C_GREEN() : "  ", is_cur ? "*" : " ",
               branches.item[i], C_DIM(), id, C_OFF());
    }
    sl_free(&branches);
    return 0;
}

static int branch_create(const char *name, int quiet)
{
    if (!ref_valid_name(name)) {
        mg_warn("'%s' is not a valid branch name", name);
        return 1;
    }

    char existing[MG_HASHSZ];
    if (ref_read(name, existing)) {
        mg_warn("branch '%s' already exists", name);
        return 1;
    }

    char id[MG_HASHSZ];
    if (!head_commit(id)) {
        mg_warn("cannot create branch '%s': no commits yet", name);
        return 1;
    }

    ref_write(name, id);
    if (!quiet) printf("Created branch '%s' at %.8s\n", name, id);
    return 0;
}

int cmd_branch(int argc, char **argv)
{
    repo_require();

    if (argc == 0) return branch_list();

    if (strcmp(argv[0], "-d") == 0 || strcmp(argv[0], "--delete") == 0) {
        if (argc < 2) { fprintf(stderr, "usage: mini_git branch -d <name>\n"); return 1; }

        char cur[MG_NAME_MAX];
        if (head_branch(cur, sizeof(cur)) && strcmp(cur, argv[1]) == 0) {
            mg_warn("cannot delete the branch you are on ('%s')", argv[1]);
            return 1;
        }
        if (ref_delete(argv[1]) != 0) {
            mg_warn("branch '%s' not found", argv[1]);
            return 1;
        }
        printf("Deleted branch '%s'\n", argv[1]);
        return 0;
    }

    return branch_create(argv[0], 0);
}

/* ------------------------------------------------------------------ */

int cmd_checkout(int argc, char **argv)
{
    repo_require();

    int force = 0, create = 0, i = 0;
    const char *target = NULL;

    for (; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0)      force = 1;
        else if (strcmp(argv[i], "-b") == 0) create = 1;
        else if (!target)                    target = argv[i];
    }

    if (!target) {
        fprintf(stderr, "usage: mini_git checkout [-b] [-f] <branch|commit>\n");
        return 1;
    }

    if (create && branch_create(target, 1) != 0) return 1;

    char id[MG_HASHSZ];
    int is_branch = ref_read(target, id);
    if (!is_branch && !commit_resolve(target, id)) {
        mg_warn("no branch or commit named '%s'", target);
        return 1;
    }

    if (!force && mg_worktree_dirty()) {
        mg_warn("you have local changes; commit them or pass -f to discard");
        return 1;
    }

    mg_switch_to(target, id, is_branch);

    if (is_branch) printf("Switched to branch '%s' (%.8s)\n", target, id);
    else           printf("HEAD is now detached at %.8s\n", id);
    return 0;
}
