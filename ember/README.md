# EMBER

EMBER is the mail-client application layer for FORGE. The supplied POSTO React UI remains the visual foundation, while its service layer now talks to the EMBER HTTP adapter and FORGE REST API.

## Architecture

```text
POSTO / React + TypeScript + Vite
        │ /api/v1 (Vite development proxy)
        ▼
EMBER backend / C++17 / port 8081
        │ HTTP/JSON with FORGE bearer token
        ▼
FORGE server / C++17 / port 8080
        ▼
FORGE services, indexes and binary storage
```

EMBER must not open FORGE `.dat` or `.idx` files or duplicate its storage logic. FORGE remains the source of truth.

## Local development

1. Build and start FORGE using `forge/README.md`.
2. Build EMBER:

   ```bash
   cmake -S ember/backend -B build/ember
   cmake --build build/ember -j2
   ./build/ember/ember-server
   ```

3. Start POSTO:

   ```bash
   cd ember/posto-—-elegant-mail-client
   npm install
   npm run dev
   ```

Vite serves the frontend on port 3000 and proxies `/api` to EMBER on port 8081. Set `FORGE_ORIGIN` to change the upstream FORGE URL.

## Implemented integration

- Real registration, login, logout and current-user lookup.
- Real message list/detail calls and a scoped FORGE query to determine recipient rows.
- Sending by exact email lookup, then FORGE's numeric-recipient message endpoint.
- Honest errors for unsupported per-user mailbox state and attachments.

## Current limitations

FORGE currently lacks per-user read/star/trash/restore state and attachment storage. Those controls are intentionally not reported as successful. Message endpoints do not return sender profiles, so the UI uses an explicit `User #ID` label for other senders rather than inventing names. Search/sort only operate on data actually returned.

The browser stores its bearer token in `sessionStorage` for this development milestone. This is not a production security design. Before deployment, move tokens to an EMBER-managed HttpOnly session cookie, add CSRF protection, HTTPS, trusted-origin validation and authentication rate limits.
