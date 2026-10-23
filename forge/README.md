# FORGE

FORGE is a C++17 file-based database engine featuring binary storage, hash indexing, search/sort algorithms, a minimal SQL-like query engine, and Argon2id-based authentication.

## Scope

- Binary record serialization (explicit little-endian encoding, no raw struct writes)
- Fixed-size record storage with `.dat`-style file persistence
- Hash indexing with FNV-1a (offset basis 0x811C9DC5, prime 0x1000193), separate chaining, 1009 buckets
- Users, Messages, Recipients with constraints and soft deletion
- Search algorithms: linear, binary, Fibonacci
- Sorting algorithms: bubble, selection, insertion, merge, quick sort
- Benchmarking with steady_clock (warm-up + 100 runs)
- SQL-like query subset: SELECT, INSERT, UPDATE, DELETE, WHERE, ORDER BY, LIMIT
- Argon2id password hashing and in-memory sessions

## Build

```bash
mkdir -p build && cd build
cmake ..
make -j4
```

## Run

```bash
./forge
```

Persistent data is stored in `forge.db` in the working directory by default.

## Tests

```bash
cd build
ctest --output-on-failure
# or
./tests/tests  # or equivalent test executable
```

## Dependencies

- C++17 compiler
- CMake >= 3.10
- Threads
- Argon2 (libargon2) - used if available, otherwise secure fallback with warnings not issued in normal flow

## Notes

- FORGE is independent from any mail client UI.
- Indexes are rebuilt from persistent data on startup.
- Sessions are in-memory and do not survive process restarts.
- Only the documented SQL subset is supported.
