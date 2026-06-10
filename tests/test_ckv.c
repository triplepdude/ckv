/* Minimal assert-based test runner for ckv. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/hashmap.h"
#include "../src/storage.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond) do {                                            \
    tests_run++;                                                    \
    if (!(cond)) {                                                  \
        tests_failed++;                                             \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                               \
} while (0)

static void test_set_get(void)
{
    hashmap *m = hm_create();
    CHECK(hm_set(m, "name", "leo", 3));
    size_t len;
    const void *v = hm_get(m, "name", &len);
    CHECK(v != NULL);
    CHECK(len == 3);
    CHECK(memcmp(v, "leo", 3) == 0);
    CHECK(hm_get(m, "missing", NULL) == NULL);
    CHECK(hm_count(m) == 1);
    hm_destroy(m);
}

static void test_overwrite(void)
{
    hashmap *m = hm_create();
    CHECK(hm_set(m, "k", "first", 5));
    CHECK(hm_set(m, "k", "second", 6));
    size_t len;
    const void *v = hm_get(m, "k", &len);
    CHECK(len == 6);
    CHECK(memcmp(v, "second", 6) == 0);
    CHECK(hm_count(m) == 1);
    hm_destroy(m);
}

static void test_delete_and_tombstones(void)
{
    hashmap *m = hm_create();
    CHECK(hm_set(m, "a", "1", 1));
    CHECK(hm_set(m, "b", "2", 1));
    CHECK(hm_delete(m, "a"));
    CHECK(!hm_delete(m, "a"));           /* already gone */
    CHECK(hm_get(m, "a", NULL) == NULL);
    CHECK(hm_get(m, "b", NULL) != NULL); /* probe chain survives deletion */
    CHECK(hm_count(m) == 1);
    /* Reinsert after delete reuses the slot. */
    CHECK(hm_set(m, "a", "3", 1));
    CHECK(hm_count(m) == 2);
    hm_destroy(m);
}

static void test_resize_many_keys(void)
{
    hashmap *m = hm_create();
    char key[32], val[32];
    const int N = 1000;

    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        snprintf(val, sizeof val, "val-%d", i);
        CHECK(hm_set(m, key, val, strlen(val)));
    }
    CHECK(hm_count(m) == (size_t)N);

    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        snprintf(val, sizeof val, "val-%d", i);
        size_t len;
        const void *v = hm_get(m, key, &len);
        CHECK(v && len == strlen(val) && memcmp(v, val, len) == 0);
    }
    hm_destroy(m);
}

static void test_binary_values(void)
{
    hashmap *m = hm_create();
    unsigned char blob[] = { 0x00, 0xFF, 0x7F, 0x00, 0x42 };
    CHECK(hm_set(m, "blob", blob, sizeof blob));
    size_t len;
    const void *v = hm_get(m, "blob", &len);
    CHECK(len == sizeof blob);
    CHECK(memcmp(v, blob, sizeof blob) == 0);
    hm_destroy(m);
}

static void test_save_load_roundtrip(void)
{
    const char *path = "test_roundtrip.ckv";
    hashmap *m = hm_create();
    CHECK(hm_set(m, "alpha", "one", 3));
    CHECK(hm_set(m, "beta", "two", 3));
    unsigned char blob[] = { 1, 2, 0, 3 };
    CHECK(hm_set(m, "blob", blob, sizeof blob));
    CHECK(store_save(m, path) == STORE_OK);
    hm_destroy(m);

    hashmap *m2 = hm_create();
    CHECK(store_load(m2, path) == STORE_OK);
    CHECK(hm_count(m2) == 3);
    size_t len;
    const void *v = hm_get(m2, "alpha", &len);
    CHECK(v && len == 3 && memcmp(v, "one", 3) == 0);
    v = hm_get(m2, "blob", &len);
    CHECK(v && len == sizeof blob && memcmp(v, blob, sizeof blob) == 0);
    hm_destroy(m2);
    remove(path);
}

static void test_load_rejects_garbage(void)
{
    const char *path = "test_garbage.ckv";
    FILE *f = fopen(path, "wb");
    fwrite("not a ckv file at all", 1, 21, f);
    fclose(f);

    hashmap *m = hm_create();
    CHECK(store_load(m, path) == STORE_ERR_FORMAT);
    hm_destroy(m);
    remove(path);
}

static void test_load_rejects_truncated(void)
{
    const char *path = "test_trunc.ckv";
    /* Valid header claiming 5 entries, but no entry data. */
    FILE *f = fopen(path, "wb");
    fwrite("CKV1", 1, 4, f);
    unsigned char count[4] = { 5, 0, 0, 0 };
    fwrite(count, 1, 4, f);
    fclose(f);

    hashmap *m = hm_create();
    CHECK(store_load(m, path) == STORE_ERR_FORMAT);
    hm_destroy(m);
    remove(path);
}

int main(void)
{
    test_set_get();
    test_overwrite();
    test_delete_and_tombstones();
    test_resize_many_keys();
    test_binary_values();
    test_save_load_roundtrip();
    test_load_rejects_garbage();
    test_load_rejects_truncated();

    printf("%d checks, %d failed\n", tests_run, tests_failed);
    return tests_failed ? 1 : 0;
}
