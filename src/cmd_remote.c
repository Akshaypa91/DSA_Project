/*
 * cmd_remote.c - push and pull against another MiniGit repository on disk.
 *
 * The original implementation shelled out to `cp -r` via system(), which is
 * non-portable, silently mangles paths containing spaces, and is a command
 * injection hazard.  Everything here is done with the portable filesystem
 * helpers in util.c instead.
 *
 * Sync model: objects are immutable and content-addressed, so copying them is
 * always safe and idempotent.  Refs are only advanced when the incoming
 * commit is a descendant of the local one, which is exactly the
 * fast-forward rule; anything else is reported and left alone.
 */
#include "minigit.h"

static void remote_dir(const char *base, char *out, size_t n)
{
    snprintf(out, n, "%s/%s", base, MG_DIR);
}

/* Copy every object from `src_objects` into `dst_objects`. */
static int sync_objects(const char *src, const char *dst)
{
    char s[MG_PATH_MAX + 64], d[MG_PATH_MAX + 64];
    snprintf(s, sizeof(s), "%s/objects", src);
    snprintf(d, sizeof(d), "%s/objects", dst);
    if (!mg_is_dir(s)) return 0;
    return mg_copy_tree(s, d);
}

static int is_repo_dir(const char *dir)
{
    char p[MG_PATH_MAX + 64];
    snprintf(p, sizeof(p), "%s/objects/commits", dir);
    return mg_is_dir(p);
}

/* Read refs/heads/<branch> out of an arbitrary .minigit directory. */
static int foreign_ref_read(const char *dir, const char *branch, char out[MG_HASHSZ])
{
    char p[MG_PATH_MAX + MG_NAME_MAX + 32];
    snprintf(p, sizeof(p), "%s/refs/heads/%s", dir, branch);
    char *txt = mg_read_file(p, NULL);
    if (!txt) return 0;
    snprintf(out, MG_HASHSZ, "%s", mg_trim(txt));
    free(txt);
    return out[0] != '\0';
}

static int foreign_ref_write(const char *dir, const char *branch, const char *id)
{
    char p[MG_PATH_MAX + MG_NAME_MAX + 32], line[MG_HASHSZ + 2];
    snprintf(p, sizeof(p), "%s/refs/heads/%s", dir, branch);
    int n = snprintf(line, sizeof(line), "%s\n", id);
    return mg_write_file(p, line, (size_t)n);
}

/* ------------------------------------------------------------------ */

int cmd_push(int argc, char **argv)
{
    repo_require();

    if (argc < 1) {
        fprintf(stderr, "usage: mini_git push <remote-path>\n");
        return 1;
    }

    char branch[MG_NAME_MAX];
    if (!head_branch(branch, sizeof(branch))) {
        mg_warn("cannot push from a detached HEAD");
        return 1;
    }

    char local_id[MG_HASHSZ];
    if (!ref_read(branch, local_id)) {
        mg_warn("branch '%s' has no commits to push", branch);
        return 1;
    }

    char remote[MG_PATH_MAX];
    remote_dir(argv[0], remote, sizeof(remote));

    if (!mg_exists(argv[0]) && mg_mkdir_p(argv[0]) != 0) {
        mg_warn("cannot create remote directory '%s'", argv[0]);
        return 1;
    }
    if (mg_mkdir_p(remote) != 0) {
        mg_warn("cannot create '%s'", remote);
        return 1;
    }

    /* Refuse to clobber a remote that has moved on without us. */
    char remote_id[MG_HASHSZ];
    if (foreign_ref_read(remote, branch, remote_id) &&
        strcmp(remote_id, local_id) != 0) {

        char local_objs[MG_PATH_MAX];
        repo_path(local_objs, sizeof(local_objs), "objects/commits/%s", remote_id);

        if (!mg_exists(local_objs) || !commit_is_ancestor(remote_id, local_id)) {
            mg_warn("remote branch '%s' is at %.8s which is not an ancestor of "
                    "%.8s -- pull first", branch, remote_id, local_id);
            return 1;
        }
    }

    if (sync_objects(MG_DIR, remote) != 0) {
        mg_warn("failed to copy objects to '%s'", remote);
        return 1;
    }
    if (foreign_ref_write(remote, branch, local_id) != 0) {
        mg_warn("failed to update remote ref");
        return 1;
    }

    printf("Pushed '%s' -> %s (now at %s%.8s%s)\n",
           branch, argv[0], C_YELLOW(), local_id, C_OFF());
    return 0;
}

/* ------------------------------------------------------------------ */

int cmd_pull(int argc, char **argv)
{
    repo_require();

    if (argc < 1) {
        fprintf(stderr, "usage: mini_git pull <remote-path>\n");
        return 1;
    }

    char remote[MG_PATH_MAX];
    remote_dir(argv[0], remote, sizeof(remote));

    if (!is_repo_dir(remote)) {
        mg_warn("'%s' does not look like a MiniGit repository", argv[0]);
        return 1;
    }

    char branch[MG_NAME_MAX];
    if (!head_branch(branch, sizeof(branch))) {
        mg_warn("cannot pull onto a detached HEAD");
        return 1;
    }

    char remote_id[MG_HASHSZ];
    if (!foreign_ref_read(remote, branch, remote_id)) {
        mg_warn("remote has no branch '%s'", branch);
        return 1;
    }

    if (sync_objects(remote, MG_DIR) != 0) {
        mg_warn("failed to copy objects from '%s'", remote);
        return 1;
    }

    char local_id[MG_HASHSZ] = "";
    int have_local = ref_read(branch, local_id);

    if (have_local && strcmp(local_id, remote_id) == 0) {
        printf("Already up to date.\n");
        return 0;
    }

    if (have_local && !commit_is_ancestor(local_id, remote_id)) {
        printf("Fetched objects, but '%s' has diverged.\n", branch);
        printf("Run: mini_git merge %s\n", remote_id);
        return 1;
    }

    if (mg_worktree_dirty()) {
        mg_warn("fetched objects, but you have local changes; "
                "commit them then re-run pull");
        return 1;
    }

    HashMap old_tree, new_tree;
    hm_init(&old_tree);
    hm_init(&new_tree);
    if (have_local) tree_of(local_id, &old_tree);
    tree_of(remote_id, &new_tree);
    tree_checkout(&old_tree, &new_tree);
    hm_free(&old_tree);
    hm_free(&new_tree);

    ref_write(branch, remote_id);

    printf("Fast-forwarded '%s' to %s%.8s%s\n",
           branch, C_YELLOW(), remote_id, C_OFF());
    return 0;
}
