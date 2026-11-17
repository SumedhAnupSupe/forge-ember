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
| FORGE | Storage validation, versioning, corruption handling | I - not yet verified | Added format.meta v1 validation/legacy adoption, fixed-width string terminator validation, and tests; not run in this environment |
| FORGE | Transactions / recovery for multi-record writes | P | Docs describe best-effort rollback only |
| FORGE | Concurrency safety | Not yet verified | Needs code review of server/storage locking |
| FORGE | CI | P | Workflow triggers only on `main` and one fix branch; no EMBER/POSTO jobs |
| EMBER | Register/login/logout/me | I | Routes exist in `backend/app/api/auth.py` |
| EMBER | Mail folders, send, read/star/trash/restore, permanent delete | I | Routes exist in `mail.py`; tests cover little |
| FORGE/EMBER | Per-user mailbox state | I - not yet verified | Added MailboxState records in format v2, migration from legacy message flags, state-aware listing/mutations, and tests; not run |
| EMBER | BCC privacy | I - not yet verified | mail.py filters BCC identities for non-senders and exposes only a BCC viewer's own address; three focused tests added, not run |
| EMBER | Pagination / server-side search | I - not yet verified | Bounded limit/offset, backend query/sort, and POSTO load-more wired; pagination is still offset-based and tests are pending |
| EMBER | Drafts | M | No draft routes |
| EMBER | Reply / reply-all / forward, threading | M | No routes or thread fields |
| EMBER | Login rate limiting, cookie sessions | M | Not present |
| EMBER | Tests | P | 2 tests |
| POSTO | Auth, folders, compose, reply UI | I - not yet verified | Removed hardcoded default/demo credentials; action failures now surface instead of falsely mutating UI; backend search and load-more wired |
| POSTO | Tests | M | No test runner configured |
| Infra | Dockerfiles, compose, `.env.example` for all services | I - not yet verified | Added FORGE/EMBER/POSTO Dockerfiles, Compose, root .env.example, .dockerignore, and full-stack CI workflow; not built/run yet |
| Docs | Root README, architecture, API docs | Not yet verified | |

## Open question for the project owner

Per-user mailbox state requires a new FORGE record type and a migration for existing `.dat` files. See the plan, task P3-1.


## Verified implementation progress

Latest verified CI run: [run 145](https://github.com/SumedhAnupSupe/forge-ember/actions/runs/43996583436). All four jobs passed:

| Job | Verification |
|---|---|
| FORGE | C++ build, CTest suite, and API smoke test |
| EMBER | Ruff and pytest (7 tests) |
| POSTO | TypeScript check and Vite production build |
| Docker | Compose configuration validation and all service image builds |

### Implemented and verified

- FORGE storage manifest v2, legacy v1/manifest-less migration, fixed-width string validation, and mailbox-state record migration.
- Restart ID restoration and \`uint32\` ID exhaustion handling.
- Checksummed message/recipient write-ahead journal, idempotent replay, and recovery of incomplete final records.
- Hash collision-chain lookup, deduplication, removal, and global-delete index cleanup.
- Independent read/star/trash state per \`(message_id, user_id)\`.
- BCC visibility enforced in FORGE response envelopes and restricted SQL query results, with a second EMBER filter.
- Authentication uses 24-hour sessions, generic invalid-credential responses, and a bounded failed-login throttle (5 failures per 15 minutes per normalized email + remote address); expired sessions and stale attempt windows are cleaned up.
- Sender/recipient message envelopes are returned in the FORGE response, removing EMBER's per-recipient HTTP lookup chain.
- Four-user API smoke coverage: sender, TO recipient, BCC recipient, unrelated user; mailbox-state isolation; per-user permanent removal; global deletion; restart.
- EMBER bounded pagination/search/sort and POSTO load-more; CC/BCC compose controls; removal of unsupported tag/attachment affordances and default demo credentials.
- Docker Compose config and all three images build in CI.

### Remaining work — not marked complete

- **P2-5:** concurrency stress tests and a deeper locking review across storage, indexes, and sessions.
- **P2-6/P2-7:** dedicated lifecycle/health tests and additional CLI behavior/exit-code coverage.
- **P3-4:** physical compaction of shared message and recipient records after the final participant permanently removes their mailbox copy.
- **P3-5:** backend login throttling and session expiry are implemented and tested. Remaining: frontend session-expiry UX and further security/error-hygiene review.
- **P4:** drafts, reply/reply-all/forward, threading, broader pagination/search tests, and performance benchmarks for batch retrieval.
- **P5:** split the large App component, add draft/thread UI, complete session-expiry and loading/empty/error UX, and add frontend component tests.
- **P6:** more failure-injection and concurrent multi-user integration scenarios.
- **P8:** measured benchmarks with environment details, reproducible demo, screenshots, and final report.

The write-ahead journal currently uses stream flush plus same-directory atomic rename; it does not issue OS-level \`fsync\` and does not cover every update/delete operation. The final shared-record purge is also not implemented. These are explicit limitations, not claimed guarantees.

## Repository hygiene and provenance

Phase 1 was already present on \`feature/full-stack-improvements\` before work continued. The branch is based on the Phase 1 audit/hygiene commit; no destructive history rewrite or main-branch change was performed. The full Git history has not been scanned for secrets.
