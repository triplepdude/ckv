#ifndef CKV_STORAGE_H
#define CKV_STORAGE_H

#include "hashmap.h"

/*
 * Binary on-disk format:
 *
 *   offset  size  field
 *   0       4     magic "CKV1"
 *   4       4     entry count (uint32, little-endian)
 *   8       ...   entries
 *
 * Each entry:
 *   4 bytes key length (uint32 LE)
 *   4 bytes value length (uint32 LE)
 *   key bytes (no NUL)
 *   value bytes
 */

typedef enum {
    STORE_OK = 0,
    STORE_ERR_IO,        /* fopen/fread/fwrite failure */
    STORE_ERR_FORMAT,    /* bad magic or truncated file */
    STORE_ERR_NOMEM,
} store_status;

store_status store_save(const hashmap *m, const char *path);
store_status store_load(hashmap *m, const char *path);

const char *store_strerror(store_status s);

#endif
