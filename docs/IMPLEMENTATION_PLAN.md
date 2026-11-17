# Implementation Plan

Order follows the master spec. Each task lists affected files, dependencies, and required tests. Status is updated as work lands; nothing is marked done until its tests have run.

## Phase 1 - Repository hygiene (done in this commit)
- Untrack `.idea/`, `*.bak`, stale `ember/backend/`; extend `.gitignore`. Fix pytest path and esbuild peer conflict.
- Tests: baseline commands in `IMPLEMENTATION_AUDIT.md` pass.

## Phase 2 - FORGE reliability
| ID | Task | Files | Depends on | Tests |
|---|---|---|---|---|
| P2-1 | Storage manifest v1, legacy adoption, bounded fixed-width string validation | `storage.cpp`, `codec.cpp`, tests | - | corruption, truncation, malformed strings, restart |
| P2-2 | ID counter restore, no wraparound | `storage.cpp` | P2-1 | ID allocation |
| P2-3 | Index update/delete correctness, collision tests, benchmark | `hash_table.cpp`, `indexes.cpp`, `benchmark.cpp` | - | collisions, rebuild, restart |
| P2-4 | Write-ahead log for message creation | `storage.cpp`, `message_service.cpp` | P2-1 | crash-between-writes recovery |
| P2-5 | Locking (reader/writer) around storage, indexes, sessions | `server.cpp`, services | P2-4 | concurrent create / ID uniqueness |
| P2-6 | Server lifecycle: startup validation, graceful shutdown, real health check | `server.cpp`, `main_server.cpp` | P2-1 | lifecycle, health |
| P2-7 | Query engine edge-case tests; CLI exit codes and help | `query.cpp`, `cli.cpp` | - | per spec 3.7 / 3.8 |

## Phase 3 - Data model and security (highest priority)

Progress: P3-3 BCC visibility filtering and focused tests are implemented but not run. P3-1, P3-2, P3-4, and P3-5 remain outstanding.
| ID | Task | Files | Depends on | Tests |
|---|---|---|---|---|
| P3-1 | `MailboxState` record (message_id, user_id, read, starred, deleted, deleted_at) + migration from per-message flags | `records.*`, `storage.cpp`, `message_service.cpp`, `server.cpp` | P2-1, P2-4 | independent state per user, migration |
| P3-2 | EMBER reads/writes per-user state; remove message-level flags from API | `mail.py`, `schemas.py`, `forge_client.py` | P3-1 | Alice/Bob/Carol scenario |
| P3-3 | BCC visibility rules in listings and detail | `mail.py` (and FORGE enrichment) | P3-1 | explicit BCC tests |
| P3-4 | Permanent delete = purge caller's copy; global purge only when no participant retains it | `server.cpp`, `mail.py` | P3-1 | multi-participant delete |
| P3-5 | Auth hardening: rate limit, expiry, error hygiene | `auth.py`, FORGE auth | - | auth negative tests |

## Phase 4 - EMBER features
Progress: bounded offset pagination and server-side search/sort are wired through EMBER and POSTO (not verified). Still required: robust filtering, drafts (P4-2), reply/reply-all/forward with in_reply_to threading field (P4-3), and batch retrieval to remove N+1 FORGE calls (P4-4). Depends on P3.

## Phase 5 - POSTO
Progress: removed default demo credentials/one-click demo accounts; mutation errors are surfaced; backend search and load-more pagination are wired. Still required: split App.tsx, session-expiry UX, CC/BCC compose controls, drafts, reply-all/threading, remove unsupported attachment/tag/wax-seal affordances, loading/error coverage, and frontend tests. Depends on P3, P4.

## Phase 6-8
Progress: added a full-stack CI workflow (FORGE, EMBER, POSTO jobs) and Dockerfiles/Compose/environment template. These have not been built or run. Still required: three-user end-to-end tests and failure scenarios; validate CI; validate Docker Compose; update all READMEs; architecture/API/database docs; benchmarks with measured environment/results; reproducible demo; screenshots; final report.

## Scope note
Phases 2-8 are several days of work. They will be delivered as separate commits on this branch, one phase at a time, each with tests run and results reported.
