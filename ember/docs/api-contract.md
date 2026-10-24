# EMBER API contract (stage 1)

Base URL: `http://127.0.0.1:8081`

| Method | Path | Upstream FORGE route | Notes |
|---|---|---|---|
| GET | `/health` | Local | EMBER process health only |
| POST | `/api/v1/users` | POST same path | Register |
| POST | `/api/v1/auth/login` | POST same path | Returns FORGE bearer token and user ID |
| POST | `/api/v1/auth/logout` | POST same path | Requires Authorization bearer |
| GET | `/api/v1/users/me` | GET same path | Requires Authorization bearer |
| GET | `/api/v1/messages` | GET same path | Requires Authorization bearer |
| GET | `/api/v1/messages/{id}` | GET same path | Requires Authorization bearer |
| POST | `/api/v1/messages` | POST same path | Requires numeric recipient IDs |
| DELETE | `/api/v1/messages/{id}` | DELETE same path | Existing FORGE deletion semantics only |

Responses preserve FORGE's current JSON shapes and HTTP status. Errors from an unavailable FORGE service return HTTP 502 with a JSON `error` field.
