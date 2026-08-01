/*
 * hashtable.c - string-keyed hash map with separate chaining.
 *
 * Buckets hold singly linked lists of HEntry.  The table doubles (and
 * rehashes) once the load factor passes 0.75, which keeps the expected
 * chain length below one and therefore lookup at O(1) average / O(n)
 * worst case.
 *
 * The same structure serves three roles in MiniGit:
 *   - tree map     : path -> blob hash
 *   - visited set  : commit id -> "1" during DAG traversal
 *   - lookup index : staged path -> hash
 */
#include "minigit.h"

#define HM_INITIAL_BUCKETS 64
#define HM_MAX_LOAD        0.75

static size_t bucket_of(const HashMap *m, const char *key)
{
    return (size_t)(mg_fnv1a_str(key) % (uint64_t)m->nbuckets);
}

void hm_init(HashMap *m)
{
    m->nbuckets = HM_INITIAL_BUCKETS;
    m->bucket   = mg_calloc(m->nbuckets, sizeof(*m->bucket));
    m->count    = 0;
}

static void hm_grow(HashMap *m)
{
    size_t old_n = m->nbuckets;
    HEntry **old = m->bucket;

    m->nbuckets = old_n * 2;
    m->bucket   = mg_calloc(m->nbuckets, sizeof(*m->bucket));

    for (size_t i = 0; i < old_n; i++) {
        HEntry *e = old[i];
        while (e) {
            HEntry *next = e->next;
            size_t b = bucket_of(m, e->key);
            e->next = m->bucket[b];
            m->bucket[b] = e;
            e = next;
        }
    }
    free(old);
}

static HEntry *hm_find(const HashMap *m, const char *key)
{
    if (!m->bucket) return NULL;
    for (HEntry *e = m->bucket[bucket_of(m, key)]; e; e = e->next)
        if (strcmp(e->key, key) == 0) return e;
    return NULL;
}

void hm_put(HashMap *m, const char *key, const char *val)
{
    if (!m->bucket) hm_init(m);

    HEntry *e = hm_find(m, key);
    if (e) {
        free(e->val);
        e->val = mg_strdup(val);
        return;
    }

    if ((double)(m->count + 1) / (double)m->nbuckets > HM_MAX_LOAD) hm_grow(m);

    size_t b = bucket_of(m, key);
    e = mg_malloc(sizeof(*e));
    e->key = mg_strdup(key);
    e->val = mg_strdup(val);
    e->next = m->bucket[b];
    m->bucket[b] = e;
    m->count++;
}

const char *hm_get(const HashMap *m, const char *key)
{
    HEntry *e = hm_find(m, key);
    return e ? e->val : NULL;
}

int hm_has(const HashMap *m, const char *key)
{
    return hm_find(m, key) != NULL;
}

void hm_del(HashMap *m, const char *key)
{
    if (!m->bucket) return;
    size_t b = bucket_of(m, key);
    HEntry *prev = NULL, *e = m->bucket[b];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            if (prev) prev->next = e->next;
            else      m->bucket[b] = e->next;
            free(e->key);
            free(e->val);
            free(e);
            m->count--;
            return;
        }
        prev = e;
        e = e->next;
    }
}

void hm_keys(const HashMap *m, StrList *out)
{
    if (!m->bucket) return;
    for (size_t i = 0; i < m->nbuckets; i++)
        for (HEntry *e = m->bucket[i]; e; e = e->next)
            sl_push(out, e->key);
    sl_sort(out);
}

double hm_load_factor(const HashMap *m)
{
    if (!m->bucket || m->nbuckets == 0) return 0.0;
    return (double)m->count / (double)m->nbuckets;
}

void hm_free(HashMap *m)
{
    if (!m->bucket) return;
    for (size_t i = 0; i < m->nbuckets; i++) {
        HEntry *e = m->bucket[i];
        while (e) {
            HEntry *next = e->next;
            free(e->key);
            free(e->val);
            free(e);
            e = next;
        }
    }
    free(m->bucket);
    m->bucket = NULL;
    m->nbuckets = m->count = 0;
}
