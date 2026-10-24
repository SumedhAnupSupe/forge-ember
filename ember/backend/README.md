# EMBER FastAPI backend

EMBER is the application API between POSTO (React/Vite) and FORGE (C++ REST database service). FORGE remains the only source of persistent data; this service does not access its binary files and does not create a second database.

## Requirements and startup

Python 3.11+, uv, and a running FORGE server are required.

From the repository root:

```bash
cd ember/backend
cp .env.example .env
uv sync --extra dev
uv run uvicorn app.main:app --host 127.0.0.1 --port 8081 --reload
```

OpenAPI docs: http://127.0.0.1:8081/docs

Start services in this order: FORGE on port 8080, EMBER on port 8081, then POSTO on port 3000.

## Routes

- GET /health and /health/live — liveness.
- GET /health/ready — checks FORGE connectivity.
- GET /api/v1/system/status — service status.
- POST /api/v1/auth/register — create a FORGE account.
- POST /api/v1/auth/login — create a FORGE session and return the user.
- POST /api/v1/auth/logout — revoke the FORGE session.
- GET /api/v1/auth/me — current user.
- GET /api/v1/messages and GET /api/v1/mailboxes/inbox — messages visible to current user.
- GET /api/v1/messages/{id} — detail; FORGE enforces access.
- POST /api/v1/messages — send mail.
- DELETE /api/v1/messages/{id} — currently sender-only deletion in FORGE.

## Send contract

```json
{"subject":"Project update","body":"The API is ready.","recipients":[{"user_id":2,"type":"TO"}]}
```

FORGE currently accepts recipient IDs, not email addresses, and does not expose per-user read/star/trash state or enriched sender/recipient details in its message-list response. EMBER deliberately does not fake those features or store mailbox state in process memory. A safe authenticated FORGE recipient lookup and richer mailbox contract are required before those UI features can be correctly connected.

## Error shape

```json
{"error":{"code":"UNAUTHORIZED","message":"Sign in to continue."}}
```

Network errors and timeouts become 503/504. Validation errors do not echo submitted values. Passwords, tokens, and message bodies are not logged.

## Tests

```bash
uv run pytest
```

Tests validate the API boundary without requiring a live FORGE instance. A real FORGE/EMBER/POSTO end-to-end test remains necessary before the whole application can be called verified.
