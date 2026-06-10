/*
 * ckv — a tiny persistent key-value store.
 *
 * Usage:
 *   ckv <file> set <key> <value>
 *   ckv <file> get <key>
 *   ckv <file> del <key>
 *   ckv <file> list
 *   ckv <file>            (interactive REPL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "storage.h"

static int load_if_exists(hashmap *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;            /* new database */
    fclose(f);

    store_status rc = store_load(m, path);
    if (rc != STORE_OK) {
        fprintf(stderr, "ckv: %s: %s\n", path, store_strerror(rc));
        return -1;
    }
    return 0;
}

static int save(hashmap *m, const char *path)
{
    store_status rc = store_save(m, path);
    if (rc != STORE_OK) {
        fprintf(stderr, "ckv: %s: %s\n", path, store_strerror(rc));
        return -1;
    }
    return 0;
}

static void cmd_list(const hashmap *m)
{
    for (long i = hm_next(m, 0); i != -1; i = hm_next(m, i + 1)) {
        const hm_entry *e = &m->entries[i];
        printf("%s = %.*s\n", e->key, (int)e->value_len, (const char *)e->value);
    }
}

/* Returns 0 on success, 1 on user error, -1 to quit the REPL. */
static int dispatch(hashmap *m, const char *path, int argc, char **argv)
{
    if (argc == 0) return 1;

    if (strcmp(argv[0], "set") == 0 && argc == 3) {
        if (!hm_set(m, argv[1], argv[2], strlen(argv[2]))) {
            fprintf(stderr, "ckv: out of memory\n");
            return 1;
        }
        return save(m, path) == 0 ? 0 : 1;
    }
    if (strcmp(argv[0], "get") == 0 && argc == 2) {
        size_t len;
        const void *v = hm_get(m, argv[1], &len);
        if (!v) {
            fprintf(stderr, "ckv: key not found: %s\n", argv[1]);
            return 1;
        }
        printf("%.*s\n", (int)len, (const char *)v);
        return 0;
    }
    if (strcmp(argv[0], "del") == 0 && argc == 2) {
        if (!hm_delete(m, argv[1])) {
            fprintf(stderr, "ckv: key not found: %s\n", argv[1]);
            return 1;
        }
        return save(m, path) == 0 ? 0 : 1;
    }
    if (strcmp(argv[0], "list") == 0 && argc == 1) {
        cmd_list(m);
        return 0;
    }
    if (strcmp(argv[0], "count") == 0 && argc == 1) {
        printf("%zu\n", hm_count(m));
        return 0;
    }
    if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
        return -1;
    }

    fprintf(stderr,
            "commands: set <k> <v> | get <k> | del <k> | list | count | quit\n");
    return 1;
}

/* Split a line into whitespace-separated tokens, in place. */
static int tokenize(char *line, char **argv, int max)
{
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && argc < max) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    return argc;
}

static int repl(hashmap *m, const char *path)
{
    char line[4096];
    char *argv[8];

    printf("ckv interactive — %zu key(s) loaded from %s\n", hm_count(m), path);
    for (;;) {
        printf("ckv> ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;
        int argc = tokenize(line, argv, 8);
        if (argc == 0) continue;
        if (dispatch(m, path, argc, argv) == -1) break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file> [set|get|del|list|count] [args]\n",
                argv[0]);
        return 2;
    }

    const char *path = argv[1];
    hashmap *m = hm_create();
    if (!m) {
        fprintf(stderr, "ckv: out of memory\n");
        return 1;
    }
    if (load_if_exists(m, path) != 0) {
        hm_destroy(m);
        return 1;
    }

    int rc;
    if (argc == 2)
        rc = repl(m, path);
    else
        rc = dispatch(m, path, argc - 2, argv + 2) == 0 ? 0 : 1;

    hm_destroy(m);
    return rc;
}
