/*
 * cmd_basic.c - init, add, rm, status
 */
#include "minigit.h"

#define DEFAULT_BRANCH "main"

/* ------------------------------------------------------------------ */

int cmd_init(int argc, char **argv)
{
    (void)argc; (void)argv;

    int reinit = repo_exists();

    char p[MG_PATH_MAX];
    if (mg_mkdir_p(MG_DIR) != 0) mg_die("cannot create %s", MG_DIR);

    repo_path(p, sizeof(p), "objects/blobs");   mg_mkdir_p(p);
    repo_path(p, sizeof(p), "objects/commits"); mg_mkdir_p(p);
    repo_path(p, sizeof(p), "refs/heads");      mg_mkdir_p(p);

    repo_path(p, sizeof(p), "index");
    if (!mg_exists(p)) mg_write_file(p, "", 0);

    repo_path(p, sizeof(p), "HEAD");
    if (!mg_exists(p)) head_set_branch(DEFAULT_BRANCH);

    printf("%s MiniGit repository in %s/ (branch '%s')\n",
           reinit ? "Reinitialised existing" : "Initialised empty",
           MG_DIR, DEFAULT_BRANCH);
    return 0;
}

/* ------------------------------------------------------------------ */

static int stage_one(Index *ix, const char *path)
{
    if (!mg_exists(path)) {
        mg_warn("pathspec '%s' does not match any file", path);
        return 0;
    }

    size_t len;
    char *data = mg_read_file(path, &len);
    if (!data) {
        mg_warn("cannot read '%s'", path);
        return 0;
    }

    char hash[MG_HASHSZ];
    int rc = blob_write(data, len, hash);
    free(data);
    if (rc != 0) {
        mg_warn("cannot store blob for '%s'", path);
        return 0;
    }

    index_enqueue(ix, path, hash);
    return 1;
}

int cmd_add(int argc, char **argv)
{
    repo_require();
    if (argc < 1) {
        fprintf(stderr, "usage: mini_git add <path>... | .\n");
        return 1;
    }

    Index ix;
    index_load(&ix);

    int staged = 0;
    for (int i = 0; i < argc; i++) {
        const char *spec = argv[i];

        if (strcmp(spec, ".") == 0 || mg_is_dir(spec)) {
            StrList files;
            sl_init(&files);
            mg_walk_worktree(&files);

            size_t plen = strlen(spec);
            for (size_t k = 0; k < files.len; k++) {
                if (strcmp(spec, ".") != 0 &&
                    !(strncmp(files.item[k], spec, plen) == 0 && files.item[k][plen] == '/'))
                    continue;
                staged += stage_one(&ix, files.item[k]);
            }
            sl_free(&files);
        } else {
            staged += stage_one(&ix, spec);
        }
    }

    index_save(&ix);
    index_free(&ix);

    printf("Staged %d file(s).\n", staged);
    return staged > 0 ? 0 : 1;
}

/* ------------------------------------------------------------------ */

int cmd_rm(int argc, char **argv)
{
    repo_require();
    if (argc < 1) {
        fprintf(stderr, "usage: mini_git rm [--cached] <path>...\n");
        return 1;
    }

    int cached = 0, first = 0;
    if (strcmp(argv[0], "--cached") == 0) { cached = 1; first = 1; }

    char head_id[MG_HASHSZ];
    HashMap tree;
    hm_init(&tree);
    if (head_commit(head_id)) tree_of(head_id, &tree);

    Index ix;
    index_load(&ix);

    int removed = 0;
    for (int i = first; i < argc; i++) {
        const char *path = argv[i];
        int tracked = hm_has(&tree, path);
        int staged  = index_lookup(&ix, path) != NULL;

        if (!tracked && !staged) {
            mg_warn("'%s' is not tracked", path);
            continue;
        }

        if (tracked) index_enqueue(&ix, path, MG_DELETED);
        else         index_remove(&ix, path);

        if (!cached && mg_exists(path)) mg_unlink(path);
        removed++;
        printf("removed '%s'\n", path);
    }

    index_save(&ix);
    index_free(&ix);
    hm_free(&tree);
    return removed > 0 ? 0 : 1;
}

/* ------------------------------------------------------------------ */

int cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    repo_require();

    char desc[MG_PATH_MAX];
    head_describe(desc, sizeof(desc));
    printf("On branch %s%s%s\n", C_BOLD(), desc, C_OFF());

    char head_id[MG_HASHSZ];
    int have_head = head_commit(head_id);

    HashMap tree;
    hm_init(&tree);
    if (have_head) tree_of(head_id, &tree);
    else printf("%sNo commits yet%s\n", C_DIM(), C_OFF());

    char merging[MG_HASHSZ];
    if (merge_head_read(merging))
        printf("%sYou have unmerged changes from %.8s -- resolve, add, then commit.%s\n",
               C_YELLOW(), merging, C_OFF());

    Index ix;
    index_load(&ix);

    /* ---- staged ---- */
    StrList staged_lines;
    sl_init(&staged_lines);
    for (IndexEntry *e = ix.front; e; e = e->next) {
        const char *old = hm_get(&tree, e->path);
        char line[MG_PATH_MAX + 32];
        if (strcmp(e->hash, MG_DELETED) == 0) {
            if (!old) continue;
            snprintf(line, sizeof(line), "deleted:   %s", e->path);
        } else if (!old) {
            snprintf(line, sizeof(line), "new file:  %s", e->path);
        } else if (strcmp(old, e->hash) != 0) {
            snprintf(line, sizeof(line), "modified:  %s", e->path);
        } else {
            continue;
        }
        sl_push(&staged_lines, line);
    }

    /* ---- unstaged ---- */
    StrList tracked;
    sl_init(&tracked);
    hm_keys(&tree, &tracked);
    for (IndexEntry *e = ix.front; e; e = e->next)
        if (!sl_has(&tracked, e->path) && strcmp(e->hash, MG_DELETED) != 0)
            sl_push(&tracked, e->path);
    sl_sort(&tracked);

    StrList unstaged;
    sl_init(&unstaged);
    for (size_t i = 0; i < tracked.len; i++) {
        const char *path = tracked.item[i];
        const char *expect = index_lookup(&ix, path);
        if (!expect) expect = hm_get(&tree, path);
        if (!expect || strcmp(expect, MG_DELETED) == 0) continue;

        char line[MG_PATH_MAX + 32], now[MG_HASHSZ];
        if (!mg_exists(path)) {
            snprintf(line, sizeof(line), "deleted:   %s", path);
            sl_push(&unstaged, line);
        } else if (mg_hash_path(path, now) == 0 && strcmp(now, expect) != 0) {
            snprintf(line, sizeof(line), "modified:  %s", path);
            sl_push(&unstaged, line);
        }
    }

    /* ---- untracked ---- */
    StrList work, untracked;
    sl_init(&work);
    sl_init(&untracked);
    mg_walk_worktree(&work);
    for (size_t i = 0; i < work.len; i++)
        if (!hm_has(&tree, work.item[i]) && !index_lookup(&ix, work.item[i]))
            sl_push(&untracked, work.item[i]);

    if (staged_lines.len) {
        printf("\nChanges to be committed:\n");
        for (size_t i = 0; i < staged_lines.len; i++)
            printf("  %s%s%s\n", C_GREEN(), staged_lines.item[i], C_OFF());
    }
    if (unstaged.len) {
        printf("\nChanges not staged for commit:\n");
        for (size_t i = 0; i < unstaged.len; i++)
            printf("  %s%s%s\n", C_RED(), unstaged.item[i], C_OFF());
    }
    if (untracked.len) {
        printf("\nUntracked files:\n");
        for (size_t i = 0; i < untracked.len; i++)
            printf("  %s%s%s\n", C_RED(), untracked.item[i], C_OFF());
    }
    if (!staged_lines.len && !unstaged.len && !untracked.len)
        printf("\nnothing to commit, working tree clean\n");

    sl_free(&staged_lines);
    sl_free(&unstaged);
    sl_free(&untracked);
    sl_free(&tracked);
    sl_free(&work);
    index_free(&ix);
    hm_free(&tree);
    return 0;
}
