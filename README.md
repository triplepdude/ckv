# ckv

A tiny persistent key-value store written in C11 with zero dependencies.

```
$ ./ckv mydb.ckv set name Leo
$ ./ckv mydb.ckv get name
Leo
$ ./ckv mydb.ckv
ckv interactive — 1 key(s) loaded from mydb.ckv
ckv> set lang C
ckv> list
name = Leo
lang = C
```

## Features

- **Custom hash map** — open addressing with linear probing, FNV-1a hashing,
  tombstone deletion, and automatic power-of-two resizing at 70% load.
- **Binary persistence** — compact length-prefixed on-disk format with a magic
  header. Values can contain arbitrary bytes, including NULs.
- **Crash safety** — saves write to a temp file and `rename()` it into place,
  so an interrupted write never corrupts an existing database.
- **Defensive loading** — corrupt or truncated files are rejected with a clear
  error instead of undefined behavior.
- **Two interfaces** — one-shot CLI commands for scripting, or an interactive
  REPL.

## Building

```
make            # build the ckv binary
make test       # build and run the test suite
make debug      # rebuild with ASan/UBSan and run tests
```

Requires only a C11 compiler (gcc or clang) and make.

## Commands

| Command            | Description                          |
|--------------------|--------------------------------------|
| `set <key> <value>`| Insert or overwrite a key            |
| `get <key>`        | Print a value                        |
| `del <key>`        | Remove a key                         |
| `list`             | Print all key/value pairs            |
| `count`            | Print the number of keys             |

## On-disk format

```
offset  size  field
0       4     magic "CKV1"
4       4     entry count (uint32, little-endian)
8       ...   entries: [key_len:u32][val_len:u32][key bytes][value bytes]
```

Integers are serialized byte-by-byte rather than memcpy'd, so files are
portable across architectures regardless of host endianness.

## Design notes

- The map stores copies of keys and values; callers never manage its memory.
- Deletion uses tombstones so probe chains stay intact; tombstones are
  reclaimed on resize.
- `hm_next()` exposes ordered slot iteration without leaking the probing
  scheme to callers.

## License

MIT
