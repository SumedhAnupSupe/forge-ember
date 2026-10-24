# FORGE Repair TODO

> Verification note (2026-10-24): GitHub Actions clean configure/build/CTest and the API smoke test passed on commit `f340a47af7e67f39226c208f76c5dcd6d16a05c6`. The branch then received a small change to defer message-index publication until all message records are written (`11bf0bfb011228cd063dbed685380cd00e754498`); that newer commit's CI result must be checked before sign-off. No local Pop!_OS build was run from this chat.

## A. Baseline and scope
- [x] Inspect repository, branch, and working-tree status.
- [x] Preserve existing uncommitted user changes.
- [x] Inspect current production source lists and CMake targets.
- [ ] Record baseline build outcome (not run in this repair session).
- [ ] Record baseline test outcome (not run in this repair session).
- [x] Inspect existing tests and identify weak or misleading tests.
- [x] Create this persistent TODO checklist.
- [x] Confirm EMBER files remain untouched.

## B. Security and authorization
- [!] Inspect every route that can access private data.
- [x] Keep SQL read-only and restricted to an allowlisted grammar.
- [x] Enforce sender/recipient checks for message reads.
- [x] Restrict message deletion to the sender.
- [x] Derive sender identity from the authenticated session.
- [x] Apply authenticated-user row filtering to queries and message routes.
- [x] Restrict users query results to the authenticated user's row.
- [x] Keep password hashes out of public query projections and API responses.
- [!] Prevent token and session-internal disclosure.
- [x] Add two-user privacy regression tests.
- [x] Add tests for query row isolation and unauthorized HTTP query access.
- [x] Add API smoke coverage for unauthorized message deletion.
- [!] Verify error responses do not leak private information.

## C. Query engine
- [x] Define and document the supported read-only SQL subset.
- [x] Reject unsupported mutation statements.
- [x] Validate table names.
- [x] Validate projected columns.
- [x] Validate WHERE columns and values.
- [x] Validate ORDER BY columns and directions.
- [x] Fix and test WHERE for users.
- [x] Fix and test WHERE for messages.
- [x] Fix and test WHERE for recipients.
- [x] Implement ascending ordering.
- [x] Implement descending ordering.
- [x] Apply filtering before ordering and limiting.
- [x] Implement safe LIMIT parsing.
- [x] Reject negative and overflowing limits.
- [x] Handle empty results correctly.
- [x] Reject unsupported syntax, multiple statements, and trailing tokens.
- [x] Remove redundant per-row storage reads from authorized message/recipient queries.
- [x] Ensure query output never exposes password hashes.
- [x] Add query parser tests.
- [x] Add query execution tests.
- [x] Add query authorization tests.
- [x] Verify query tests pass in GitHub Actions CTest.

## D. Storage and data integrity
- [ ] Verify existing encoded record sizes and field layout.
- [ ] Preserve field-by-field binary serialization.
- [ ] Preserve existing .dat file compatibility.
- [ ] Distinguish empty results from storage errors.
- [ ] Validate file-open and seek operations.
- [ ] Validate record-file sizes.
- [ ] Check full-record reads.
- [ ] Check write and flush outcomes.
- [ ] Propagate storage errors through services.
- [x] Validate message and recipient data before writing.
- [x] Reject duplicate recipients and cap recipients per message.
- [ ] Implement and verify rollback for failed multi-record writes.
- [ ] Report rollback failure accurately.
- [x] Restore ID counters by scanning persisted records; test ID allocation after reopen.
- [x] Prevent ID overflow and reuse; add exhaustion tests.
- [x] Serialize shared storage stream access and ID allocation with a recursive mutex.
- [x] Defer message/recipient index publication until all records are written; update user indexes only after successful storage changes.
- [x] Add corruption/truncation test confirming an invalid existing file is not truncated.
- [x] Test storage reopen and API account persistence across server restart.
- [ ] Add failed-write and rollback tests.
- [ ] Add concurrency tests.
- [x] Run storage tests using isolated temporary directories in CTest.

## E. Authentication
- [x] Require Argon2id with no silent fallback.
- [x] Verify Argon2id headers and linking.
- [x] Fix secure random generation to handle partial reads and EINTR.
- [x] Fail closed if secure randomness is unavailable.
- [x] Synchronize shared session state.
- [x] Verify session expiry behavior.
- [x] Verify logout invalidation.
- [x] Ensure failed hashing/randomness never bypasses authentication.
- [x] Repair tests that swallow unexpected exceptions.
- [x] Add password verification tests.
- [x] Add invalid, expired, and logged-out token tests.
- [ ] Add session concurrency tests.
- [x] Run all authentication tests.

## F. REST API and server lifecycle
- [x] Make server startup status accurate.
- [ ] Handle bind/listen failure.
- [x] Implement real server shutdown.
- [ ] Ensure in-flight requests finish before storage closes.
- [x] Default to loopback binding.
- [x] Support configured host and port.
- [x] Validate registration and message payload field types and limits.
- [x] Standardize HTTP status codes and JSON errors.
- [x] Implement or verify registration.
- [x] Implement or verify login.
- [x] Require a valid session for logout and test unauthenticated logout rejection.
- [x] Implement or verify current-user lookup.
- [x] Define message listing as sent plus received messages.
- [x] Implement or verify authorized message listing.
- [x] Implement authorized single-message retrieval.
- [x] Implement validated message creation.
- [x] Implement deletion according to the documented policy.
- [x] Keep unsafe generic SQL disabled for ordinary users.
- [x] Add HTTP API smoke tests; verify their latest CI result before sign-off.
- [ ] Test server startup failure and shutdown.
- [x] Test account login and profile retrieval after restarting the API server with the same data directory.
- [x] Run the automated API smoke test covering auth, schema, query restrictions, message access, deletion, logout, and restart persistence.

## G. CLI
- [x] Ensure CLI communicates over HTTP only.
- [x] Ensure CLI does not access storage files directly.
- [x] Implement help.
- [x] Implement status.
- [x] Implement login.
- [x] Implement logout.
- [x] Implement whoami.
- [x] Fetch authorized SHOW TABLES metadata through REST.
- [x] Fetch authorized DESCRIBE metadata through REST.
- [x] Send SELECT queries to the authenticated REST endpoint and render returned rows.
- [x] Implement safe session handling.
- [x] Avoid password echo where supported.
- [x] Avoid printing tokens.
- [x] Sanitize terminal control characters.
- [x] Bound table widths and long output.
- [x] Distinguish errors from empty results.
- [x] Handle connection failure and malformed responses.
- [x] Handle EOF and exit cleanly.
- [x] Support configurable server address.
- [ ] Add CLI parser tests.
- [ ] Add CLI rendering/error tests.
- [ ] Add end-to-end CLI/server tests.

## H. Build system and repository hygiene
- [ ] Verify forge_core target composition.
- [ ] Verify forge-server target.
- [ ] Verify forge-cli target.
- [x] Verify dependency linking and Argon2id configuration.
- [x] Ensure tests link to the intended core implementation.
- [x] Add root-level build-directory ignore rules.
- [x] Identify tracked generated build artifacts.
- [x] Remove generated artifacts from Git tracking.
- [x] Preserve actual source and dependency files.
- [x] Inspect Git diff and status after cleanup.
- [x] Verify EMBER changes are absent.

## I. Documentation
- [x] Document actual architecture.
- [x] Document dependencies.
- [x] Document clean build commands.
- [x] Document server startup and shutdown.
- [x] Document host, port, and data-directory configuration.
- [x] Document CLI commands.
- [x] Document supported SQL and unsupported operations.
- [x] Document REST endpoints and response codes.
- [x] Document authentication and authorization rules.
- [x] Document storage format and persistence.
- [x] Document test commands.
- [x] Document known limitations accurately.
- [x] Remove obsolete executable, storage, and test instructions.

## J. Final verification and report
- [x] Configure from a clean build directory.
- [x] Build every required target.
- [x] Run CTest with failure output enabled.
- [ ] Confirm the API smoke test passes on the latest branch commit.
- [ ] Run CLI/server end-to-end tests.
- [x] Review test output for skipped or misleading tests.
- [x] Verify existing storage files were not overwritten or migrated.
- [x] Verify no tracked generated build artifacts remain.
- [x] Inspect the final diff for unrelated changes.
- [x] Confirm EMBER remains untouched.
- [x] Mark only genuinely verified TODO items complete.
- [x] Document remaining limitations (best-effort transactions; restricted SQL subset).
- [x] Provide exact commands run and their actual outcomes.
- [x] Do not push or merge changes.
