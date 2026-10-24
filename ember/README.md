# EMBER

EMBER is the mail-client application layer. Its frontend talks only to the EMBER HTTP API; EMBER talks to FORGE over HTTP/JSON. Neither frontend nor EMBER accesses FORGE's binary storage files.

## Layout

- `frontend/`: React + TypeScript + Vite mail-client UI (add the supplied POSTO frontend here).
- `backend/`: C++17 HTTP API / backend-for-frontend.

## Backend setup

Requires CMake 3.16+, a C++17 compiler, and network access on first configure to fetch cpp-httplib and nlohmann/json.

```bash
cmake -S ember/backend -B build/ember
cmake --build build/ember -j2
FORGE_URL=http://127.0.0.1:8080 EMBER_HOST=127.0.0.1 EMBER_PORT=8090 ./build/ember/ember-server
```

Check `GET http://127.0.0.1:8090/health`. Start FORGE first.

## Implemented API proxy routes

- `GET /health` — reports EMBER status and whether FORGE's health endpoint responded.
- `POST /api/v1/auth/register` — forwards registration to FORGE.
- `POST /api/v1/auth/login` — forwards login; FORGE remains the token authority.
- `POST /api/v1/auth/logout` — forwards the bearer token to FORGE.
- `GET /api/v1/auth/me` — forwards to FORGE's `/api/v1/users/me`.
- `GET /api/v1/messages`, `GET /api/v1/messages/:id`, `POST /api/v1/messages`, `DELETE /api/v1/messages/:id` — forwards supported message operations.

EMBER preserves FORGE's status codes and response JSON as much as possible. A missing/unreachable FORGE service produces a 502 response. No SQL writes are performed through the restricted query endpoint.

## Deliberate scope limits

The existing FORGE schema has record-level `is_read`, `is_starred`, and `is_deleted` flags on a message. Using those as per-user mailbox state would make one recipient's actions affect another recipient's view. Therefore starred/read/trash/restore/permanent-delete mailbox routes are not faked in this first backend skeleton. They require a deliberate per-user state contract in FORGE before they can be safely implemented.

Attachments are not stored by the current FORGE API; this version does not pretend to upload/download attachment bytes.

## Frontend

Place the provided POSTO React/Vite project inside `ember/frontend/`. Replace its mock/localStorage implementation in `src/services/api.ts` with calls to the EMBER API. Configure Vite's dev proxy to forward `/api` to `http://127.0.0.1:8090`. The source ZIP supplied during planning is not automatically committed by the backend setup; add its files in the frontend folder before treating the UI integration as complete.
