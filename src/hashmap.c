#include "hashmap.h"

#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MAX_LOAD_PERCENT 70

/* Sentinel for deleted slots; never dereferenced as a string. */
static char TOMBSTONE_SENTINEL;
#define TOMBSTONE (&TOMBSTONE_SENTINEL)

/* strdup is POSIX, not C11 — provide a portable copy. */
static char *dup_string(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

unsigned long long hm_hash(const char *key)
{
    /* FNV-1a 64-bit */
    unsigned long long h = 1469598103934665603ULL;
    for (const unsigned char *p = (const unsigned char *)key; *p; p++) {
        h ^= *p;
        h *= 1099511628211ULL;
    }
    return h;
}

hashmap *hm_create(void)
{
    hashmap *m = malloc(sizeof *m);
    if (!m) return NULL;
    m->entries = calloc(INITIAL_CAPACITY, sizeof *m->entries);
    if (!m->entries) { free(m); return NULL; }
    m->capacity = INITIAL_CAPACITY;
    m->count = 0;
    m->tombstones = 0;
    return m;
}

static void free_entry(hm_entry *e)
{
    if (e->key && e->key != TOMBSTONE) {
        free(e->key);
        free(e->value);
    }
    e->key = NULL;
    e->value = NULL;
    e->value_len = 0;
}

void hm_destroy(hashmap *m)
{
    if (!m) return;
    for (size_t i = 0; i < m->capacity; i++)
        free_entry(&m->entries[i]);
    free(m->entries);
    free(m);
}

/* Find slot for key: either its current slot or the first usable empty one. */
static hm_entry *find_slot(hm_entry *entries, size_t capacity, const char *key)
{
    size_t mask = capacity - 1;
    size_t i = (size_t)(hm_hash(key) & mask);
    hm_entry *first_tombstone = NULL;

    for (;;) {
        hm_entry *e = &entries[i];
        if (e->key == NULL)
            return first_tombstone ? first_tombstone : e;
        if (e->key == TOMBSTONE) {
            if (!first_tombstone) first_tombstone = e;
        } else if (strcmp(e->key, key) == 0) {
            return e;
        }
        i = (i + 1) & mask;
    }
}

static bool resize(hashmap *m, size_t new_capacity)
{
    hm_entry *fresh = calloc(new_capacity, sizeof *fresh);
    if (!fresh) return false;

    for (size_t i = 0; i < m->capacity; i++) {
        hm_entry *e = &m->entries[i];
        if (e->key && e->key != TOMBSTONE) {
            hm_entry *dst = find_slot(fresh, new_capacity, e->key);
            *dst = *e;
        }
    }
    free(m->entries);
    m->entries = fresh;
    m->capacity = new_capacity;
    m->tombstones = 0;
    return true;
}

bool hm_set(hashmap *m, const char *key, const void *value, size_t value_len)
{
    if ((m->count + m->tombstones + 1) * 100 >= m->capacity * MAX_LOAD_PERCENT) {
        if (!resize(m, m->capacity * 2))
            return false;
    }

    hm_entry *e = find_slot(m->entries, m->capacity, key);
    bool existing = e->key && e->key != TOMBSTONE;

    void *vcopy = malloc(value_len ? value_len : 1);
    if (!vcopy) return false;
    memcpy(vcopy, value, value_len);

    if (existing) {
        free(e->value);
    } else {
        char *kcopy = dup_string(key);
        if (!kcopy) { free(vcopy); return false; }
        if (e->key == TOMBSTONE) m->tombstones--;
        e->key = kcopy;
        m->count++;
    }
    e->value = vcopy;
    e->value_len = value_len;
    return true;
}

const void *hm_get(const hashmap *m, const char *key, size_t *value_len)
{
    hm_entry *e = find_slot(m->entries, m->capacity, key);
    if (!e->key || e->key == TOMBSTONE) return NULL;
    if (value_len) *value_len = e->value_len;
    return e->value;
}

bool hm_delete(hashmap *m, const char *key)
{
    hm_entry *e = find_slot(m->entries, m->capacity, key);
    if (!e->key || e->key == TOMBSTONE) return false;
    free(e->key);
    free(e->value);
    e->key = TOMBSTONE;
    e->value = NULL;
    e->value_len = 0;
    m->count--;
    m->tombstones++;
    return true;
}

size_t hm_count(const hashmap *m)
{
    return m->count;
}

long hm_next(const hashmap *m, long start)
{
    for (size_t i = (size_t)start; i < m->capacity; i++) {
        if (m->entries[i].key && m->entries[i].key != TOMBSTONE)
            return (long)i;
    }
    return -1;
}
