/*
 * object.c - the object store and the commit DAG.
 *
 * Two object kinds live under .minigit/objects:
 *
 *   blobs   - file contents, addressed by the hash of the bytes.  Two files
 *             with identical content share one blob, so repeated commits of
 *             an unchanged file cost nothing.  Stored as
 *             objects/blobs/<first 2 hex>/<remaining hex> to keep any one
 *             directory small.
 *
 *   commits - plain-text metadata, addressed by the hash of that text.  A
 *             commit therefore has a deterministic id derived from its
 *             parents, timestamp, message and full tree.
 *
 * Commit serialisation:
 *
 *     parent  <id | ->
 *     merge   <id | ->
 *     time    <unix epoch>
 *     message <one line, \n and \\ escaped>
 *     tree
 *     <blob hash> <path>
 *     ...
 *
 * Parent pointers make the history a singly linked list in the common case
 * and a DAG once merges appear.  Traversals below use an explicit queue plus
 * a hash-table visited set, so a diamond history is never walked twice.
 */
#include "minigit.h"

/* ------------------------------------------------------------------ *
 * Blobs
 * ------------------------------------------------------------------ */

static void blob_path(const char *hash, char *out, size_t n)
{
    repo_path(out, n, "objects/blobs/%.2s/%s", hash, hash + 2);
}

int blob_exists(const char *hash)
{
    char p[MG_PATH_MAX];
    blob_path(hash, p, sizeof(p));
    return mg_exists(p);
}

int blob_write(const void *data, size_t len, char out[MG_HASHSZ])
{
    mg_hash_data(data, len, out);

    char p[MG_PATH_MAX];
    blob_path(out, p, sizeof(p));
    if (mg_exists(p)) return 0; /* deduplicated */

    return mg_write_file(p, data, len);
}

char *blob_read(const char *hash, size_t *out_len)
{
    if (!hash || strcmp(hash, MG_DELETED) == 0) return NULL;
    char p[MG_PATH_MAX];
    blob_path(hash, p, sizeof(p));
    return mg_read_file(p, out_len);
}

/* ------------------------------------------------------------------ *
 * Commit serialisation helpers
 * ------------------------------------------------------------------ */

static void escape_msg(const char *in, Buffer *b)
{
    for (const char *p = in; *p; p++) {
        if (*p == '\\')      buf_puts(b, "\\\\");
        else if (*p == '\n') buf_puts(b, "\\n");
        else if (*p == '\r') continue;
        else                 buf_append(b, p, 1);
    }
}

static void unescape_msg(const char *in, char *out, size_t n)
{
    size_t o = 0;
    for (const char *p = in; *p && o + 1 < n; p++) {
        if (*p == '\\' && p[1] == 'n')      { out[o++] = '\n'; p++; }
        else if (*p == '\\' && p[1] == '\\'){ out[o++] = '\\'; p++; }
        else                                 { out[o++] = *p; }
    }
    out[o] = '\0';
}

static void commit_path(const char *id, char *out, size_t n)
{
    repo_path(out, n, "objects/commits/%s", id);
}

int commit_exists(const char *id)
{
    if (!id || !*id) return 0;
    char p[MG_PATH_MAX];
    commit_path(id, p, sizeof(p));
    return mg_exists(p);
}

int commit_store(const char *parent, const char *parent2, const char *message,
                 time_t ts, const HashMap *tree, char out[MG_HASHSZ])
{
    Buffer b;
    buf_init(&b);
    buf_printf(&b, "parent  %s\n", (parent  && *parent)  ? parent  : "-");
    buf_printf(&b, "merge   %s\n", (parent2 && *parent2) ? parent2 : "-");
    buf_printf(&b, "time    %lld\n", (long long)ts);
    buf_puts(&b, "message ");
    escape_msg(message, &b);
    buf_puts(&b, "\ntree\n");

    StrList paths;
    sl_init(&paths);
    hm_keys(tree, &paths); /* sorted => deterministic id */
    for (size_t i = 0; i < paths.len; i++)
        buf_printf(&b, "%s %s\n", hm_get(tree, paths.item[i]), paths.item[i]);
    sl_free(&paths);

    mg_hash_data(b.data, b.len, out);

    char p[MG_PATH_MAX];
    commit_path(out, p, sizeof(p));
    int rc = mg_exists(p) ? 0 : mg_write_file(p, b.data, b.len);
    buf_free(&b);
    return rc;
}

int commit_load(const char *id, Commit *out, HashMap *tree)
{
    if (!id || !*id) return 0;

    char p[MG_PATH_MAX];
    commit_path(id, p, sizeof(p));
    char *txt = mg_read_file(p, NULL);
    if (!txt) return 0;

    if (out) {
        memset(out, 0, sizeof(*out));
        snprintf(out->id, MG_HASHSZ, "%s", id);
    }

    int in_tree = 0;
    char *save = txt, *line = txt;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        char *t = line;

        if (in_tree) {
            char *sp = strchr(t, ' ');
            if (sp && tree) {
                *sp = '\0';
                hm_put(tree, mg_trim(sp + 1), mg_trim(t));
            }
        } else if (strcmp(mg_trim(t), "tree") == 0) {
            in_tree = 1;
        } else if (strncmp(t, "parent", 6) == 0) {
            char *v = mg_trim(t + 6);
            if (out && strcmp(v, "-") != 0) snprintf(out->parent, MG_HASHSZ, "%s", v);
        } else if (strncmp(t, "merge", 5) == 0) {
            char *v = mg_trim(t + 5);
            if (out && strcmp(v, "-") != 0) snprintf(out->parent2, MG_HASHSZ, "%s", v);
        } else if (strncmp(t, "time", 4) == 0) {
            if (out) out->timestamp = (time_t)strtoll(mg_trim(t + 4), NULL, 10);
        } else if (strncmp(t, "message", 7) == 0) {
            if (out) unescape_msg(t + 8, out->message, sizeof(out->message));
        }
        line = nl ? nl + 1 : NULL;
    }
    free(save);
    return 1;
}

int tree_of(const char *commit_id, HashMap *tree)
{
    if (!commit_id || !*commit_id) return 0;
    return commit_load(commit_id, NULL, tree);
}

/* ------------------------------------------------------------------ *
 * Reference resolution: branch name, full id, or unique id prefix
 * ------------------------------------------------------------------ */

int commit_resolve(const char *ref, char out[MG_HASHSZ])
{
    if (!ref || !*ref) return 0;

    if (strcmp(ref, "HEAD") == 0) return head_commit(out);

    if (ref_read(ref, out)) return 1;

    if (commit_exists(ref)) {
        snprintf(out, MG_HASHSZ, "%s", ref);
        return 1;
    }

    /* unique prefix */
    char dir[MG_PATH_MAX];
    repo_path(dir, sizeof(dir), "objects/commits");
    StrList ids;
    sl_init(&ids);
    mg_list_dir(dir, &ids);

    size_t rl = strlen(ref);
    int matches = 0;
    char found[MG_HASHSZ] = "";
    for (size_t i = 0; i < ids.len; i++) {
        if (strncmp(ids.item[i], ref, rl) == 0) {
            matches++;
            snprintf(found, MG_HASHSZ, "%s", ids.item[i]);
        }
    }
    sl_free(&ids);

    if (matches == 1) { snprintf(out, MG_HASHSZ, "%s", found); return 1; }
    if (matches > 1)  mg_warn("ambiguous reference '%s' matches %d commits", ref, matches);
    return 0;
}

/* ------------------------------------------------------------------ *
 * DAG traversal
 * ------------------------------------------------------------------ */

/*
 * Generation number: the length of the longest path from a commit back to a
 * root.  A child's depth is always strictly greater than either parent's, so
 * depth is a valid topological rank.
 *
 * MiniGit's timestamps only have one-second resolution, which means several
 * commits made in quick succession compare equal.  Ordering history or
 * picking a merge base by timestamp alone therefore produces wrong answers
 * on fast automated runs; depth breaks those ties correctly.
 *
 * Computed with an explicit stack (not recursion) so that a long linear
 * history cannot overflow the C stack.  Memoised, so O(V + E) overall.
 */
void commit_depths(const char *id, HashMap *out)
{
    if (!id || !*id) return;

    StrList stack;
    sl_init(&stack);
    sl_push(&stack, id);

    while (stack.len > 0) {
        char cur[MG_HASHSZ];
        snprintf(cur, sizeof(cur), "%s", stack.item[stack.len - 1]);

        if (hm_has(out, cur)) {           /* already resolved */
            free(stack.item[--stack.len]);
            continue;
        }

        Commit c;
        if (!commit_load(cur, &c, NULL)) { /* missing object: treat as root */
            hm_put(out, cur, "0");
            free(stack.item[--stack.len]);
            continue;
        }

        int pending = 0;
        if (c.parent[0]  && !hm_has(out, c.parent))  { sl_push(&stack, c.parent);  pending = 1; }
        if (c.parent2[0] && !hm_has(out, c.parent2)) { sl_push(&stack, c.parent2); pending = 1; }
        if (pending) continue;

        long best = -1;
        if (c.parent[0]) {
            long d = strtol(hm_get(out, c.parent), NULL, 10);
            if (d > best) best = d;
        }
        if (c.parent2[0]) {
            long d = strtol(hm_get(out, c.parent2), NULL, 10);
            if (d > best) best = d;
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", best + 1);
        hm_put(out, cur, buf);
        free(stack.item[--stack.len]);
    }

    sl_free(&stack);
}

static long depth_of(const HashMap *depths, const char *id)
{
    const char *v = hm_get(depths, id);
    return v ? strtol(v, NULL, 10) : 0;
}

/* Newest-first linked list of every commit reachable from `id`. */
Commit *commit_history(const char *id)
{
    HashMap depths;
    hm_init(&depths);
    commit_depths(id, &depths);

    HashMap seen;
    hm_init(&seen);

    StrList queue; /* BFS frontier */
    sl_init(&queue);
    Commit *head = NULL;

    if (id && *id) { sl_push(&queue, id); hm_put(&seen, id, "1"); }

    for (size_t qi = 0; qi < queue.len; qi++) {
        Commit tmp;
        if (!commit_load(queue.item[qi], &tmp, NULL)) continue;

        Commit *c = mg_malloc(sizeof(*c));
        *c = tmp;
        long cd = depth_of(&depths, c->id);

        /* Insert newest first, breaking timestamp ties by depth so a child
         * never sorts below its own parent. */
        if (!head ||
            c->timestamp > head->timestamp ||
            (c->timestamp == head->timestamp && cd > depth_of(&depths, head->id))) {
            c->next = head;
            head = c;
        } else {
            Commit *p = head;
            while (p->next &&
                   (p->next->timestamp > c->timestamp ||
                    (p->next->timestamp == c->timestamp &&
                     depth_of(&depths, p->next->id) >= cd)))
                p = p->next;
            c->next = p->next;
            p->next = c;
        }

        if (c->parent[0]  && !hm_has(&seen, c->parent))  { hm_put(&seen, c->parent, "1");  sl_push(&queue, c->parent); }
        if (c->parent2[0] && !hm_has(&seen, c->parent2)) { hm_put(&seen, c->parent2, "1"); sl_push(&queue, c->parent2); }
    }

    sl_free(&queue);
    hm_free(&seen);
    hm_free(&depths);
    return head;
}

void commit_free_list(Commit *head)
{
    while (head) {
        Commit *next = head->next;
        free(head);
        head = next;
    }
}

/* Fill `set` with every commit reachable from `id`, inclusive. */
void commit_ancestors(const char *id, HashMap *set)
{
    StrList queue;
    sl_init(&queue);

    if (id && *id) { sl_push(&queue, id); hm_put(set, id, "1"); }

    for (size_t qi = 0; qi < queue.len; qi++) {
        Commit c;
        if (!commit_load(queue.item[qi], &c, NULL)) continue;

        if (c.parent[0]  && !hm_has(set, c.parent))  { hm_put(set, c.parent, "1");  sl_push(&queue, c.parent); }
        if (c.parent2[0] && !hm_has(set, c.parent2)) { hm_put(set, c.parent2, "1"); sl_push(&queue, c.parent2); }
    }
    sl_free(&queue);
}

int commit_is_ancestor(const char *maybe_ancestor, const char *descendant)
{
    if (!maybe_ancestor || !*maybe_ancestor) return 1; /* empty history */
    if (!descendant || !*descendant) return 0;
    if (strcmp(maybe_ancestor, descendant) == 0) return 1;

    HashMap set;
    hm_init(&set);
    commit_ancestors(descendant, &set);
    int yes = hm_has(&set, maybe_ancestor);
    hm_free(&set);
    return yes;
}

/*
 * Best common ancestor ("merge base") of two commits.
 *
 * Collect the ancestors of `a` into a hash set, then intersect with the
 * ancestors of `b` and keep the deepest common commit.  Depth is the
 * generation number computed above, so this is a true lowest common
 * ancestor: no other common ancestor can be a descendant of it.
 *
 * O(V + E) time and O(V) space, dominated by the two traversals.
 */
int commit_merge_base(const char *a, const char *b, char out[MG_HASHSZ])
{
    HashMap anc_a;
    hm_init(&anc_a);
    commit_ancestors(a, &anc_a);

    HashMap anc_b;
    hm_init(&anc_b);
    commit_ancestors(b, &anc_b);

    HashMap depths;
    hm_init(&depths);
    commit_depths(a, &depths);
    commit_depths(b, &depths);

    StrList keys;
    sl_init(&keys);
    hm_keys(&anc_b, &keys);

    long best_depth = -1;
    int found = 0;
    for (size_t i = 0; i < keys.len; i++) {
        if (!hm_has(&anc_a, keys.item[i])) continue;
        long d = depth_of(&depths, keys.item[i]);
        if (d > best_depth) {
            best_depth = d;
            snprintf(out, MG_HASHSZ, "%s", keys.item[i]);
            found = 1;
        }
    }

    sl_free(&keys);
    hm_free(&depths);
    hm_free(&anc_a);
    hm_free(&anc_b);
    return found;
}

/* ------------------------------------------------------------------ *
 * Working-tree materialisation
 * ------------------------------------------------------------------ */

void tree_checkout(const HashMap *old_tree, const HashMap *new_tree)
{
    /* Remove files that were tracked before but are absent from the target. */
    if (old_tree) {
        StrList old_paths;
        sl_init(&old_paths);
        hm_keys(old_tree, &old_paths);
        for (size_t i = 0; i < old_paths.len; i++)
            if (!hm_has(new_tree, old_paths.item[i]))
                mg_unlink(old_paths.item[i]);
        sl_free(&old_paths);
    }

    StrList paths;
    sl_init(&paths);
    hm_keys(new_tree, &paths);
    for (size_t i = 0; i < paths.len; i++) {
        const char *hash = hm_get(new_tree, paths.item[i]);
        size_t len = 0;
        char *data = blob_read(hash, &len);
        if (!data) {
            mg_warn("missing blob %s for %s", hash, paths.item[i]);
            continue;
        }
        if (mg_write_file(paths.item[i], data, len) != 0)
            mg_warn("cannot write %s", paths.item[i]);
        free(data);
    }
    sl_free(&paths);
}
