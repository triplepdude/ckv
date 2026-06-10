#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const char MAGIC[4] = { 'C', 'K', 'V', '1' };

static void put_u32(uint8_t out[4], uint32_t v)
{
    out[0] = (uint8_t)(v);
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

static uint32_t get_u32(const uint8_t in[4])
{
    return (uint32_t)in[0]
         | ((uint32_t)in[1] << 8)
         | ((uint32_t)in[2] << 16)
         | ((uint32_t)in[3] << 24);
}

store_status store_save(const hashmap *m, const char *path)
{
    /* Write to a temp file then rename, so a crash mid-write
     * never corrupts an existing database. */
    size_t plen = strlen(path);
    char *tmp = malloc(plen + 5);
    if (!tmp) return STORE_ERR_NOMEM;
    memcpy(tmp, path, plen);
    memcpy(tmp + plen, ".tmp", 5);

    FILE *f = fopen(tmp, "wb");
    if (!f) { free(tmp); return STORE_ERR_IO; }

    store_status rc = STORE_OK;
    uint8_t buf[4];

    if (fwrite(MAGIC, 1, 4, f) != 4) { rc = STORE_ERR_IO; goto done; }
    put_u32(buf, (uint32_t)hm_count(m));
    if (fwrite(buf, 1, 4, f) != 4) { rc = STORE_ERR_IO; goto done; }

    for (long i = hm_next(m, 0); i != -1; i = hm_next(m, i + 1)) {
        const hm_entry *e = &m->entries[i];
        uint32_t klen = (uint32_t)strlen(e->key);
        uint32_t vlen = (uint32_t)e->value_len;

        put_u32(buf, klen);
        if (fwrite(buf, 1, 4, f) != 4) { rc = STORE_ERR_IO; goto done; }
        put_u32(buf, vlen);
        if (fwrite(buf, 1, 4, f) != 4) { rc = STORE_ERR_IO; goto done; }
        if (fwrite(e->key, 1, klen, f) != klen) { rc = STORE_ERR_IO; goto done; }
        if (vlen && fwrite(e->value, 1, vlen, f) != vlen) { rc = STORE_ERR_IO; goto done; }
    }

done:
    if (fclose(f) != 0 && rc == STORE_OK) rc = STORE_ERR_IO;
    if (rc == STORE_OK && rename(tmp, path) != 0) rc = STORE_ERR_IO;
    if (rc != STORE_OK) remove(tmp);
    free(tmp);
    return rc;
}

store_status store_load(hashmap *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return STORE_ERR_IO;

    store_status rc = STORE_OK;
    char *key = NULL;
    uint8_t *val = NULL;
    uint8_t buf[4];

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        rc = STORE_ERR_FORMAT;
        goto done;
    }
    if (fread(buf, 1, 4, f) != 4) { rc = STORE_ERR_FORMAT; goto done; }
    uint32_t count = get_u32(buf);

    for (uint32_t i = 0; i < count; i++) {
        if (fread(buf, 1, 4, f) != 4) { rc = STORE_ERR_FORMAT; goto done; }
        uint32_t klen = get_u32(buf);
        if (fread(buf, 1, 4, f) != 4) { rc = STORE_ERR_FORMAT; goto done; }
        uint32_t vlen = get_u32(buf);

        key = malloc((size_t)klen + 1);
        val = malloc(vlen ? vlen : 1);
        if (!key || !val) { rc = STORE_ERR_NOMEM; goto done; }

        if (fread(key, 1, klen, f) != klen) { rc = STORE_ERR_FORMAT; goto done; }
        key[klen] = '\0';
        if (vlen && fread(val, 1, vlen, f) != vlen) { rc = STORE_ERR_FORMAT; goto done; }

        if (!hm_set(m, key, val, vlen)) { rc = STORE_ERR_NOMEM; goto done; }
        free(key); key = NULL;
        free(val); val = NULL;
    }

done:
    free(key);
    free(val);
    fclose(f);
    return rc;
}

const char *store_strerror(store_status s)
{
    switch (s) {
    case STORE_OK:         return "ok";
    case STORE_ERR_IO:     return "I/O error";
    case STORE_ERR_FORMAT: return "corrupt or invalid database file";
    case STORE_ERR_NOMEM:  return "out of memory";
    }
    return "unknown error";
}
