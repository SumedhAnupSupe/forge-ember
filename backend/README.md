# EMBER FastAPI backend

## Run locally

1. Start FORGE with its existing build instructions at http://127.0.0.1:8080.
2. From this directory run `uv sync`.
3. Copy `.env.example` to `.env` if you need to change FORGE/CORS settings.
4. Run `uv run uvicorn app.main:app --reload --host 127.0.0.1 --port 8000`.
5. Open http://127.0.0.1:8000/docs.

The Vite frontend runs on port 3000 and uses `VITE_API_BASE_URL=http://127.0.0.1:8000/api/v1`.

FORGE remains the source of truth; FastAPI does not open FORGE storage files directly. Attachments, tags, and wax-seal metadata are not persisted by the current FORGE message schema.
