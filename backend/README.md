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

FORGE format version 2 stores read/star/trash flags in `mailbox_states.dat`, keyed by `(message_id, user_id)`. The version 1 to version 2 startup migration initializes a state row for the sender and each active recipient using the old message-level flags. EMBER filters BCC recipient identities per viewer and supports bounded mailbox `limit`/`offset`, search, and sorting. Permanent deletion currently removes only the requesting user's mailbox-state row; physical compaction of the shared message and recipient records after the last participant removes their copy is still outstanding. Message creation also remains best-effort rollback rather than crash-safe WAL recovery.
