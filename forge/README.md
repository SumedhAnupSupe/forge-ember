# FORGE

FORGE is a standalone C++17 database service with binary storage, indexing, a restricted SQL-like query engine, Argon2id authentication, REST API, and a separate HTTP CLI client.

## Architecture

- `forge_core` (library): models, codecs, storage, indexes, services, auth, algorithms, query logic
- `forge-server`: HTTP server (REST API), uses `forge_core` and same service layer
- `forge-cli`: interactive terminal HTTP client (no direct storage access)

EMBER backend communicates with FORGE via the REST API. The CLI and EMBER use the same API and service layer.

## Dependencies

- C++17 compiler
- CMake >= 3.10
- Threads
- libargon2 and `argon2.h` (required; CMake configuration fails if either is unavailable).

## Build

```bash
cmake -S forge -B build/forge
cmake --build build/forge --clean-first -j2
ctest --test-dir build/forge --output-on-failure
```

## Run server

```bash
./build/forge/forge-server --host 127.0.0.1 --port 8080 --data-dir ./forge/data
```

Data is stored in `.dat` files under the data directory: `users.dat`, `messages.dat`, `recipients.dat`. Existing files are not migrated or overwritten silently.

## Run CLI

```bash
./build/forge/forge-cli --host 127.0.0.1 --port 8080
```

CLI connects over HTTP only; it does not read/write `.dat` files directly.

## CLI commands

- `help`, `status`, `login`, `logout`, `whoami`
- `SHOW TABLES;`
- `DESCRIBE users;` / `DESCRIBE messages;` / `DESCRIBE recipients;`
- `SELECT` queries are temporarily disabled in the CLI until a safe, authorized read-only query API is implemented.
- `exit`, `quit`

## Query API status

The internal query engine supports a restricted read-only SQL subset, but the HTTP query endpoint deliberately returns `403 Forbidden` until row-level authorization is implemented. The CLI therefore does not send SQL queries to that endpoint. Do not treat the internal query engine as a public API.

```sql
SELECT * FROM users;
SELECT id, name, email FROM users WHERE id = 1;
SELECT id, subject FROM messages ORDER BY created_at DESC LIMIT 10;
SELECT message_id, user_id FROM recipients WHERE message_id = 5;
```

Rules:
- Tables: `users`, `messages`, `recipients`
- `SELECT` with explicit columns or `*` (password hashes are never exposed)
- At most one equality `WHERE` on allowed columns
- `ORDER BY` with `ASC`/`DESC` (or default ASC), stable sort
- `LIMIT` must be non-negative integer, no overflow
- `INSERT`/`UPDATE`/`DELETE` are rejected
- Joins, multiple statements, arbitrary operators unsupported
- Unknown tables/columns/trailing tokens/invalid limits are rejected
- Empty result is success
- Generic SQL access through public API is disabled for ordinary users (returns `403 Forbidden`)

## REST endpoints

- `GET /health`
- `POST /api/v1/users`
- `POST /api/v1/auth/login`
- `POST /api/v1/auth/logout`
- `GET /api/v1/users/me`
- `GET /api/v1/messages` (messages accessible to authenticated user)
- `GET /api/v1/messages/{id}` (only if sender or authorized recipient)
- `POST /api/v1/messages`
- `DELETE /api/v1/messages/{id}` (sender only)
- `POST /api/v1/query` (disabled until safe row-level authorization is implemented → `403 Forbidden`)

Status codes: `200`, `201`, `400`, `401`, `403`, `404`, `409`, `500`. Errors do not leak stack traces/secrets.

## Authentication & authorization

- Argon2id password hashing required
- Secure random tokens (from `getrandom`), fail closed if unavailable
- Session state synchronized with mutex; expired sessions rejected
- Failed login does not reveal whether email exists
- Sender identity derived from session; client-supplied `sender_id` not trusted
- Message access restricted to sender or authorized recipients
- Password hashes/sessions/secrets never returned in API/CLI/logs

## Storage

- Files: `users.dat`, `messages.dat`, `recipients.dat`
- Record sizes (verified): User 461 bytes, Message 659 bytes, Recipient 13 bytes
- Field-by-field binary serialization (no raw struct writes)
- Storage errors distinguished from empty results where possible
- Multi-record message writes use validation and best-effort rollback on failure (documented as best-effort, not full ACID)
- IDs allocated safely; indexes updated only after successful storage operations

## Tests

```bash
ctest --test-dir build/forge --output-on-failure
```

Tests use temporary directories where appropriate; corruption tests must not run against real data.

## Known limitations

- Best-effort rollback only (no write-ahead log/crash-safe transactions)
- Generic SQL via public API disabled for ordinary users
- Per-user message deletion design depends on schema; current schema uses soft deletion at record level
- CLI keeps tokens in memory only (not written to disk)
