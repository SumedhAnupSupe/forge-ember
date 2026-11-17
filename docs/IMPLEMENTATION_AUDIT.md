# Implementation Audit (Phase 0)

Branch: `feature/full-stack-improvements`, based on `main` @ `08dc087`.
Method: built and ran each component, then inspected routes and data-model code by search.
Anything marked **Not yet verified** has not been exercised by me and must not be treated as working.

## Baseline results (measured)

| Component | Command | Result before changes | Result after Phase 1 |
|---|---|---|---|
| FORGE | `cmake` configure + build (Release) | Builds cleanly | same |
| FORGE | `ctest` (`forge_tests`) | 1/1 passed | same |
| EMBER | `uv run pytest` | **Collection error**: `No module named 'app'` | 2 passed |
| POSTO | `npm ci` | **Fails**: ERESOLVE, `esbuild ^0.25` vs vite 8 peer `^0.27 \|\| ^0.28` | installs (esbuild bumped, lockfile regenerated) |
| POSTO | `npm run lint` (`tsc --noEmit`) / `npm run build` | blocked by install failure | pass / builds |

Fixes applied: `[tool.pytest.ini_options] pythonpath` in `backend/pyproject.toml`; `esbuild` devDependency bumped `^0.25.0` -> `^0.27.0` (the version range vite 8 declares as its peer) with `package-lock.json` regenerated. No other dependency versions were changed deliberately; the lockfile regeneration may have moved transitive versions within their ranges.
Not run yet: FORGE API smoke test (`forge/tests/api_smoke.sh`), live end-to-end through all three services.

## Duplicate directories

- `backend/` is the live EMBER backend (used by `start-ember.sh`, CI-relevant).
- `ember/backend/` contained only a `uv.lock` (different from `backend/uv.lock`) and a `.env` using variable names (`EMBER_HOST`, `EMBER_CORS_ORIGINS`, ...) that the current `backend/app/config.py` does not read. Nothing references it except a `.idea` exclude entry. Classified as a stale leftover and removed from tracking. The `.env` held only local URLs/ports, no credentials.

## Repository hygiene

| Item | Status |
|---|---|
| `.idea/` tracked | Was tracked; untracked, ignored |
| `*.bak` files (3) tracked | Removed |
| `ember/backend/.env` tracked | Removed; `.env` ignored |
| Committed secrets | None found in the tracked `.env`. Full history not scanned yet |
| Root `.gitignore` | Extended (IDE, logs, node_modules, dist) |

## Feature classification

Legend: **W** working (verified), **I** implemented, not verified, **P** partial, **M** missing, **B** broken, **A** needs architectural change.

| Area | Feature | Status | Evidence |
|---|---|---|---|
| FORGE | Build, unit tests | W | Section above |
| FORGE | Read-only SELECT endpoint, auth row filtering | I | `REPAIR_TODO.md` claims; not re-verified |
| FORGE | Storage validation, versioning, corruption handling | Not yet verified | `REPAIR_TODO.md` section D is unchecked |
| FORGE | Transactions / recovery for multi-record writes | P | Docs describe best-effort rollback only |
| FORGE | Concurrency safety | Not yet verified | Needs code review of server/storage locking |
| FORGE | CI | P | Workflow triggers only on `main` and one fix branch; no EMBER/POSTO jobs |
| EMBER | Register/login/logout/me | I | Routes exist in `backend/app/api/auth.py` |
| EMBER | Mail folders, send, read/star/trash/restore, permanent delete | I | Routes exist in `mail.py`; tests cover little |
| EMBER | Per-user mailbox state | **A** | `is_read`/`is_starred`/`is_deleted` live on the message record (`server.cpp` PATCH routes, `schemas.py`). One user's action changes every participant's view |
| EMBER | BCC privacy | Not yet verified, likely B | Message enrichment returns recipient lists; no visibility filtering seen in `mail.py` |
| EMBER | Pagination / server-side search | M | `GET /{folder}` returns a plain list and filters in Python |
| EMBER | Drafts | M | No draft routes |
| EMBER | Reply / reply-all / forward, threading | M | No routes or thread fields |
| EMBER | Login rate limiting, cookie sessions | M | Not present |
| EMBER | Tests | P | 2 tests |
| POSTO | Auth, folders, compose, reply UI | I | Single `App.tsx`; not yet exercised against a live backend |
| POSTO | Tests | M | No test runner configured |
| Infra | Dockerfiles, compose, `.env.example` for all services | M | Only `backend/.env.example` exists |
| Docs | Root README, architecture, API docs | Not yet verified | |

## Open question for the project owner

Per-user mailbox state requires a new FORGE record type and a migration for existing `.dat` files. See the plan, task P3-1.
