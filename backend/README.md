# EMBER FastAPI backend

## Run locally

1. Start FORGE with its existing build instructions at http://127.0.0.1:8080.
2. From this directory run `uv sync`.
3. Copy `.env.example` to `.env` if you need to change FORGE/CORS settings.
4. Run `uv run uvicorn app.main:app --reload --host 127.0.0.1 --port 8000`.
5. Open http://127.0.0.1:8000/docs.

The Vite frontend runs on port 3000 and uses `VITE_API_BASE_URL=http://127.0.0.1:8000/api/v1`.

FORGE login sessions expire after 24 hours and failed login attempts are throttled per normalized email and remote address. FORGE remains the source of truth; FastAPI does not open FORGE storage files directly. Attachments, tags, and wax-seal metadata are not persisted by the current FORGE message schema.

## Known FORGE storage constraints

FORGE format version 2 stores read/star/trash flags in `mailbox_states.dat`, keyed by `(message_id, user_id)`. The version 1 to version 2 startup migration initializes a state row for the sender and each active recipient using the old message-level flags. EMBER filters BCC recipient identities per viewer and supports bounded mailbox `limit`/`offset`, search, and sorting. Permanent deletion removes only the requesting user's mailbox-state row. When it is the final participant, FORGE compacts the shared message, recipient, and mailbox-state records through a replayable purge journal. Message plus recipient creation uses a checksummed, replayable FORGE write-ahead journal; it does not cover every mutation and does not issue OS-level `fsync`.
