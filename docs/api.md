# API Overview

EMBER is the browser-facing API. OpenAPI is available at `/docs` when the backend is running.

## EMBER routes

| Method | Route | Purpose |
|---|---|---|
| POST | `/api/v1/auth/register` | Register a user through FORGE and establish a session |
| POST | `/api/v1/auth/login` | Authenticate and return user/session |
| POST | `/api/v1/auth/logout` | End the FORGE session |
| GET | `/api/v1/auth/me` | Return the current authenticated user |
| GET | `/api/v1/mail/{folder}` | List inbox, sent, starred, or trash |
| GET | `/api/v1/mail/item/{message_id}` | Read an authorized message |
| POST | `/api/v1/mail` | Send a message to TO/CC/BCC recipients |
| PATCH | `/api/v1/mail/{message_id}/read` | Change read state |
| PATCH | `/api/v1/mail/{message_id}/star` | Change starred state |
| DELETE | `/api/v1/mail/{message_id}` | Move message to trash |
| POST | `/api/v1/mail/{message_id}/restore` | Restore message from trash |
| DELETE | `/api/v1/mail/{message_id}/permanent` | Current route delegates to FORGE delete; physical purge is not yet implemented |

## Mailbox query parameters

`GET /api/v1/mail/{folder}` accepts:

- `q`: optional search text, maximum 200 characters.
- `sort`: `newest` or `oldest`.
- `limit`: 1-100, default 50.
- `offset`: 0-1,000,000, default 0.

The response remains a JSON list for compatibility. POSTO requests pages using offset/limit.

## BCC response policy

The sender sees the complete recipient envelope. A TO/CC recipient sees TO/CC entries but no BCC identities. A BCC recipient sees TO/CC and only their own BCC entry.

## FORGE API

EMBER uses FORGE's authenticated REST API for users, messages, recipient lookup, and the restricted read-only query endpoint. The query API is not general SQL; unsupported statements and syntax are rejected.

## Known contract gaps

- Mailbox state is still message-level and therefore not independent per user.
- Draft, reply-all, forwarding, and thread APIs are not implemented.
- The permanent-delete route does not yet guarantee physical erasure.
