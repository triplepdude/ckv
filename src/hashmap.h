#ifndef CKV_HASHMAP_H
#define CKV_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

/*
 * Open-addressing hash map (linear probing) with string keys and
 * arbitrary binary values. Keys and values are copied on insert;
 * the map owns its memory.
 */

typedef struct {
    char *key;          /* NULL = empty, TOMBSTONE = deleted */
    void *value;
    size_t value_len;
} hm_entry;

typedef struct {
    hm_entry *entries;
    size_t capacity;    /* always a power of two */
    size_t count;       /* live entries */
    size_t tombstones;
} hashmap;

hashmap *hm_create(void);
void hm_destroy(hashmap *m);

/* Returns false on allocation failure. Overwrites existing keys. */
bool hm_set(hashmap *m, const char *key, const void *value, size_t value_len);

/* Returns pointer to internal value (do not free) or NULL if absent. */
const void *hm_get(const hashmap *m, const char *key, size_t *value_len);

/* Returns true if the key existed and was removed. */
bool hm_delete(hashmap *m, const char *key);

size_t hm_count(const hashmap *m);

/* Iteration: returns next live index >= start, or -1 when exhausted. */
long hm_next(const hashmap *m, long start);

/* FNV-1a 64-bit, exposed for testing. */
unsigned long long hm_hash(const char *key);

#endif
