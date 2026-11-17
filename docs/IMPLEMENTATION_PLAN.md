# Implementation Plan

Order follows the master spec. Each task lists affected files, dependencies, and required tests. Status is updated as work lands; nothing is marked done until its tests have run.

## Phase 1 - Repository hygiene (done in this commit)
- Untrack `.idea/`, `*.bak`, stale `ember/backend/`; extend `.gitignore`. Fix pytest path and esbuild peer conflict.
- Tests: baseline commands in `IMPLEMENTATION_AUDIT.md` pass.

## Phase 2 - FORGE reliability
| ID | Task | Files | Depends on | Tests |
|---|---|---|---|---|
| P2-1 | Versioned storage manifest v2, legacy adoption, bounded fixed-width string validation, per-user state migration, incomplete-tail recovery | `storage.cpp`, `codec.cpp`, tests | - | corruption, truncation, malformed strings, restart |
| P2-2 | ID counter restore and no wraparound | `storage.cpp`, tests | P2-1 | restart ID scan and `uint32` exhaustion |
| P2-3 | Index update/delete correctness and collision-chain behavior | `hash_table.cpp`, `indexes.cpp`, tests | - | collision lookup, deduplication, deletion, rebuild |
| P2-4 | Versioned, checksummed write-ahead journal for message + recipient creation; idempotent replay and incomplete-tail repair | `storage.cpp`, `message_service.cpp`, tests | P2-1 | partial-write recovery, replay idempotency |
| P2-5 | Synchronization across storage, indexes, and sessions | `storage.cpp`, `indexes.cpp`, auth, tests | P2-4 | 8-thread concurrent create and unique ID test |
| P2-6 | Startup validation, health endpoint, and process shutdown/restart smoke coverage | `server.cpp`, `main_server.cpp`, CI | P2-1 | API smoke startup, health, restart |
| P2-7 | Query-engine visibility/edge-case tests | `query.cpp`, tests | - | sender, recipient, BCC, unrelated user |

## Phase 3 - Data model and security (highest priority)

Progress: P3-1/P3-2 are implemented and tested: MailboxState record, v2 migration, independent per-user list/read/star/trash state, and permanent removal of one mailbox-state row. P3-3 is implemented and covered by the four-user API smoke test plus SQL visibility tests. P3-4 is implemented and verified with a replayable purge journal: caller-copy removal leaves other participants intact, and the final participant's removal physically compacts message, recipient, and mailbox-state records. Unit and API smoke tests cover the behavior, interrupted purge replay, and restart non-resurrection. P3-5 is implemented and tested: 24-hour session expiry, bounded failed-login throttling (5 failures/15 minutes per normalized email + remote address), generic invalid-credential responses, and stale session cleanup.
| ID | Task | Files | Depends on | Tests |
|---|---|---|---|---|
| P3-1 | `MailboxState` record (message_id, user_id, read, starred, deleted) + v2 migration from per-message flags | `records.*`, `codec.*`, `storage.*`, `message_service.cpp` | P2-1 | independent state per user, migration |
| P3-2 | EMBER/FORGE reads and writes per-user state; state-aware mailbox listing and mutation | `server.cpp`, `message_service.cpp`, `mail.py` | P3-1 | Alice/Bob/Carol scenario |
| P3-3 | BCC visibility rules in listings and detail | `mail.py` (and FORGE enrichment) | P3-1 | explicit BCC tests |
| P3-4 | Permanent delete = purge caller's copy; global purge only when no participant retains it | `server.cpp`, `mail.py` | P3-1 | multi-participant delete |
| P3-5 | Auth hardening: rate limit, expiry, error hygiene | `auth.py`, FORGE auth | - | auth negative tests |

## Phase 4 - EMBER features
Progress: bounded offset pagination, server-side search/sort, POSTO load-more, and batched sender/recipient envelopes are implemented. The per-recipient HTTP lookup chain is removed. Still required: broader pagination/search tests, drafts (P4-2), reply/reply-all/forward with an `in_reply_to` threading field (P4-3), and measured performance/request-level state caching (P4-4). Depends on P3.

## Phase 5 - POSTO
Progress: removed default demo credentials/one-click demo accounts; mutation errors are surfaced; CC/BCC compose controls, search, load-more pagination, and production TypeScript build are verified. Unsupported tag/attachment affordances were removed. Still required: split App.tsx, session-expiry UX, draft persistence, reply-all/threading, fuller loading/empty/error coverage, and frontend component tests. Depends on P3, P4.

## Phase 6-8
Progress: four CI jobs pass (FORGE build/unit/API smoke, EMBER lint/tests, POSTO typecheck/build, Docker Compose config and image builds). API smoke covers sender, TO, BCC, unrelated user, mailbox state isolation, permanent per-user removal, global deletion, and restart. Still required: additional failure-injection/concurrency scenarios; measured benchmarks; reproducible demo and screenshots; final report. Root, FORGE, and EMBER READMEs plus architecture/API/database/testing docs are maintained.

## Scope note
Phases 2-8 remain in progress. P2-1 through P2-4 and P3-1 through P3-3 have passing automated coverage. Remaining items above are deliberately not marked complete until implemented and verified.
