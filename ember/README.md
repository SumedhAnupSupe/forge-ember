# EMBER

EMBER is the mail-client application layer for the FORGE database project. The supplied POSTO React UI remains the visual foundation; this folder adds a C++ HTTP adapter and a clearer frontend/backend boundary.

## Architecture

```text
Browser (POSTO / React + TypeScript)
       │ HTTP/JSON
       ▼
EMBER backend (C++17, port 8081)
       │ HTTP/JSON; forwards the FORGE bearer token
       ▼
FORGE server (C++17, port 8080)
       │
       ▼
FORGE services, indexes and binary storage
```

EMBER must not open FORGE `.dat` or `.idx` files and must not duplicate FORGE storage or authentication logic. FORGE remains the source of truth.

## Local development

1. Build and start FORGE using the instructions in `../forge/README.md`.
2. Build EMBER backend:

   ```bash
   cmake -S ember/backend -B build/ember
   cmake --build build/ember -j2
   ./build/ember/ember-server
   ```

3. Start the POSTO frontend:

   ```bash
   cd ember/posto-—-elegant-mail-client
   npm install
   npm run dev
   ```

The frontend uses `VITE_EMBER_API_URL` (default `http://127.0.0.1:8081`). The backend uses `FORGE_ORIGIN` (default `http://127.0.0.1:8080`).

## Current implementation boundary

The adapter forwards the currently implemented FORGE auth and message routes. It deliberately does not fabricate per-user read/star/trash state, attachment handling, recipient-name lookup, or restore/permanent-delete operations that FORGE does not yet expose. Those require explicit FORGE schema/API work before the UI can promise them.

## Security notes

This is a local-development adapter, not a production deployment. It currently forwards bearer tokens from the browser and allows the local Vite origin. For production, move FORGE tokens behind an EMBER-managed HttpOnly cookie session, add CSRF protection, configure trusted origins, use HTTPS, and rate-limit authentication routes.
