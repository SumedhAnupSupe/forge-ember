# EMBER / FORGE API contract

The frontend uses the Vite same-origin `/api/v1` path. EMBER forwards supported calls to FORGE while preserving upstream status codes and JSON.

| Method | Path | Status |
|---|---|---|
| POST | `/auth/login` | Forwarded to FORGE; token is kept in browser session storage for local development |
| POST | `/auth/logout` | Forwarded with bearer token |
| POST | `/users` | Registration |
| GET | `/users/me` | Current user |
| GET | `/users/lookup?email=...` | Authenticated exact-email recipient lookup added in FORGE |
| GET | `/messages` | Messages visible to current user |
| GET | `/messages/:id` | Authorized message detail |
| POST | `/messages` | Create message using numeric recipient IDs |
| POST | `/query` | FORGE restricted read-only query API, used to fetch caller-visible recipient rows |
| DELETE | `/messages/:id` | FORGE's existing sender-only delete semantics; not equivalent to recipient trash |

The frontend service resolves email recipients through the exact-email lookup route and submits numeric IDs to FORGE.

## Unsupported

Per-user read/unread, star, recipient-specific trash/restore/permanent-delete and attachments are not implemented in FORGE. EMBER must return/communicate an unsupported-feature error rather than simulate persistence. The current development token storage is not suitable for production.
