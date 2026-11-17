# FORGE + EMBER + POSTO

A full-stack mail application built around **FORGE**, a custom C++17 database engine. **EMBER** exposes the database operations through a Python FastAPI service, while **POSTO** provides the React and TypeScript mail interface.

The repository is organized as three cooperating services:

| Component | Technology | Responsibility | Local address |
|---|---|---|---|
| FORGE | C++17, CMake, HTTP/REST | Binary database storage, indexing, authentication, query execution and data API | `127.0.0.1:8080` |
| EMBER | Python 3.11+, FastAPI, uv | Application API and HTTP client for FORGE | `127.0.0.1:8000` |
| POSTO | React 19, TypeScript, Vite | Mail client UI | `localhost:3000` |

> FORGE is the source of truth for persistent user and message data. EMBER communicates with FORGE through its HTTP API; it does not read FORGE's binary storage files directly.

## Features

### FORGE database engine
- Custom binary record storage for users, messages and recipients.
- Field-by-field record serialization and storage services.
- Hash-based indexing and lookup support.
- User registration, login, session handling and Argon2id password hashing.
- HTTP server and a separate interactive HTTP CLI.
- Authenticated, restricted, read-only SQL-like `SELECT` query endpoint.
- C++ test suite covering storage, codecs, services, authentication, indexing, queries and integration.

### EMBER API
- FastAPI service between the frontend and FORGE.
- Registration, login, logout and current-user endpoints.
- Mailbox operations for inbox, sent, starred and trash.
- Send, retrieve, search, sort, mark-read, star, trash and restore operations.
- Interactive OpenAPI documentation at `/docs`.

### POSTO mail client
- React + TypeScript frontend served by Vite.
- Connects to EMBER through the configured API base URL.
- Uses the EMBER API for authentication and mail operations.

## Quick start

### Requirements

On Ubuntu/Debian, the root launcher installs the system build dependencies and, if missing, Node.js 22 LTS. It also installs `uv` for Python dependency management.

The launcher expects:
- Ubuntu/Debian with `apt-get` and `sudo` (for automatic system dependency installation).
- A C++17 compiler and CMake.
- `libargon2-dev` (FORGE requires Argon2id).
- Python 3.11 or newer.
- Node.js 22.12+ and npm.
- `curl`, `git` and `ca-certificates`.

### 1. Clone the repository

```bash
git clone https://github.com/SumedhAnupSupe/forge-ember.git
cd forge-ember
```

### 2. Start the complete application

```bash
chmod +x start.sh
./start.sh
```

You can also run it without changing the executable bit:

```bash
bash start.sh
```

On the first run, `start.sh`:
1. Installs the required Ubuntu/Debian system packages (unless disabled).
2. Installs `uv` if it is not already available.
3. Installs Node.js 22 LTS and npm if they are missing.
4. Creates `backend/.env` from `backend/.env.example` if needed.
5. Installs the EMBER Python dependencies with `uv sync`.
6. Installs POSTO dependencies with `npm ci` (or `npm install` if no lockfile exists).
7. Configures and builds FORGE with CMake.
8. Runs the FORGE test suite.
9. Starts FORGE, waits for its health endpoint, then starts EMBER.
10. Runs the POSTO TypeScript check and starts the Vite development server.

When all services are ready, open:

| Service | URL |
|---|---|
| POSTO application | http://localhost:3000 |
| EMBER Swagger / OpenAPI docs | http://127.0.0.1:8000/docs |
| EMBER health | http://127.0.0.1:8000/health |
| FORGE health | http://127.0.0.1:8080/health |

Keep the launcher terminal open while using the application. Press **Ctrl+C** to stop all three services.

### Launcher options

The launcher supports these environment variables:

| Variable | Default | Purpose |
|---|---:|---|
| `INSTALL_SYSTEM_DEPS` | `1` | Set to `0` to skip apt package installation. |
| `RUN_FORGE_TESTS` | `1` | Set to `0` to skip FORGE's CTest suite. |

For example, when system dependencies are already installed:

```bash
INSTALL_SYSTEM_DEPS=0 ./start.sh
```

## Manual development setup

Use these instructions if you want to run, debug or rebuild each component separately instead of using the all-in-one launcher.

### 1. Build and run FORGE

Install a C++17 compiler, CMake, pthread support and Argon2 development headers/library.

From the repository root:

```bash
cmake -S forge -B build/forge -DCMAKE_BUILD_TYPE=Release
cmake --build build/forge --parallel 2
ctest --test-dir build/forge --output-on-failure
```

Start the HTTP server:

```bash
mkdir -p forge/data
./build/forge/forge-server --host 127.0.0.1 --port 8080 --data-dir ./forge/data
```

In another terminal, the interactive CLI can connect to the running server:

```bash
./build/forge/forge-cli --host 127.0.0.1 --port 8080
```

The CLI is an HTTP client; it does not access the binary data files directly.

### 2. Run EMBER

```bash
cd backend
cp .env.example .env
uv sync
uv run uvicorn app.main:app --reload --host 127.0.0.1 --port 8000
```

Configuration in `backend/.env`:

```dotenv
FORGE_BASE_URL=http://127.0.0.1:8080
FRONTEND_ORIGINS=http://localhost:3000,http://127.0.0.1:3000
REQUEST_TIMEOUT_SECONDS=5
```

EMBER expects FORGE to be running and reachable at `FORGE_BASE_URL`.

### 3. Run POSTO

In a separate terminal:

```bash
cd "ember/posto-—-elegant-mail-client"
npm ci
npm run dev
```

The frontend defaults to `http://127.0.0.1:8000/api/v1` for the EMBER API. To override it, create a local `.env.local` file:

```dotenv
VITE_API_BASE_URL=http://127.0.0.1:8000/api/v1
```

Available frontend scripts:

```bash
npm run dev
npm run lint
npm run build
npm run preview
```

## API overview

### EMBER routes

All application routes are prefixed with `/api/v1`.

| Method | Route | Purpose |
|---|---|---|
| POST | `/auth/register` | Create an account |
| POST | `/auth/login` | Log in |
| POST | `/auth/logout` | End the current session |
| GET | `/auth/me` | Get the authenticated user |
| GET | `/mail/{folder}` | List inbox, sent, starred or trash |
| GET | `/mail/item/{message_id}` | Retrieve a message |
| POST | `/mail` | Send a message |
| PATCH | `/mail/{message_id}/read` | Update read state |
| PATCH | `/mail/{message_id}/star` | Update starred state |
| DELETE | `/mail/{message_id}` | Move a message to trash |
| POST | `/mail/{message_id}/restore` | Restore a message |
| DELETE | `/mail/{message_id}/permanent` | Request permanent deletion |

Health endpoints:
- `GET /health` — EMBER process health.
- `GET /health/forge` — checks connectivity to FORGE.

For the authoritative interactive API schema, use http://127.0.0.1:8000/docs while EMBER is running.

### FORGE REST API

FORGE exposes its own API on port `8080`. The core routes include user registration and lookup, authentication, session operations, message operations, schema metadata and the restricted query endpoint. See [forge/README.md](forge/README.md) for the detailed route list, query grammar, storage format and CLI commands.

## Data, logs and configuration

- **Persistent database files:** `forge/data/` (`users.dat`, `messages.dat`, `recipients.dat` are created/managed by FORGE).
- **Launcher logs:** `.run-logs/forge.log`, `.run-logs/backend.log`, `.run-logs/frontend.log`.
- **EMBER configuration:** `backend/.env` (created from the example on first launch).
- **POSTO API configuration:** `VITE_API_BASE_URL` in the frontend environment.

The launcher creates build output under `build/forge`. Runtime data under `forge/data/` persists when the services are stopped and restarted. Back up this directory before experimenting with storage changes.

## Important implementation notes

- FORGE is the persistence layer; do not edit its `.dat` files manually.
- The SQL-like query endpoint is intentionally restricted to authenticated, read-only `SELECT` statements. It is not a general-purpose SQL database.
- Current FORGE message records store read, starred and deleted flags on the message itself rather than as per-user mailbox state. Those flags are therefore shared across participants in the current storage model.
- The current delete operation is a soft delete. The permanent-delete API path should not be treated as a guarantee of physical erasure until the storage layer implements and verifies physical purge.
- The current message schema does not persist frontend attachment, tag or wax-seal metadata.

These are known storage/model limitations, not features that should be assumed to work as independent per-user mailbox state.

## Repository layout

```text
forge-ember/
├── start.sh
├── forge/                              # C++17 database engine and HTTP server
│   ├── include/
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── backend/                            # EMBER FastAPI application
│   ├── app/
│   │   ├── api/
│   │   └── clients/
│   ├── tests/
│   ├── pyproject.toml
│   └── .env.example
├── ember/
│   └── posto-—-elegant-mail-client/    # POSTO React/Vite frontend
├── build/                              # Generated CMake build output
├── .run-logs/                          # Generated launcher logs
└── README.md
```

## License

See [forge/LICENSE](forge/LICENSE) for the FORGE component license.
