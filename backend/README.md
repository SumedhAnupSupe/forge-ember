# EMBER FastAPI backend

## Run locally

1. Start FORGE with its existing build instructions at http://127.0.0.1:8080.
2. From this directory run `uv sync`.
3. Copy `.env.example` to `.env` if you need to change FORGE/CORS settings.
4. Run `uv run uvicorn app.main:app --reload --host 127.0.0.1 --port 8000`.
5. Open http://127.0.0.1:8000/docs.

The Vite frontend runs on port 3000 and uses `VITE_API_BASE_URL=http://127.0.0.1:8000/api/v1`.

FORGE remains the source of truth; FastAPI does not open FORGE storage files directly. Attachments, tags, and wax-seal metadata are not persisted by the current FORGE message schema.

## Known FORGE storage constraints

The existing FORGE record format stores `is_read`, `is_starred`, and `is_deleted` on the message record, rather than per-user mailbox state. The current mutation routes therefore authorize the caller but these three flags remain shared across participants. FORGE's current delete service is a soft-delete operation; the permanent-delete route still needs a physical-purge implementation before it can honestly guarantee erasure. These are storage-layer limitations, not frontend mocks, and should be resolved before treating multi-recipient trash/read/star behavior as production-ready.
