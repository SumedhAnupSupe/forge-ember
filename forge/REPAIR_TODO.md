# FORGE Repair TODO

> Verification note: this checklist must reflect executed evidence. No configure/build/CTest/HTTP/CLI end-to-end run was performed in this repair session; do not mark those checks complete based on code inspection alone.

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
- [x] Disable unrestricted SQL access for ordinary users.
- [!] Implement or centralize message read authorization.
- [!] Implement or centralize message deletion authorization.
- [!] Derive sender identity from the authenticated session.
- [!] Prevent user-ID or message-ID authorization bypass.
- [!] Ensure user listings return only permitted records.
- [!] Prevent password-hash disclosure.
- [!] Prevent token and session-internal disclosure.
- [!] Add two-user privacy regression tests.
- [!] Add tests for unauthorized query access.
- [!] Add tests for unauthorized message deletion.
- [!] Verify error responses do not leak private information.

## C. Query engine
- [ ] Define and document the supported SQL grammar.
- [ ] Reject unsupported mutation statements.
- [ ] Validate table names.
- [ ] Validate projected columns.
- [ ] Validate WHERE columns and values.
- [ ] Validate ORDER BY columns and directions.
- [ ] Fix WHERE for users.
- [ ] Fix WHERE for messages.
- [ ] Fix WHERE for recipients.
- [ ] Implement correct ascending ordering.
- [ ] Implement correct descending ordering.
- [ ] Apply filtering before ordering and limiting.
- [ ] Implement safe LIMIT parsing.
- [ ] Reject negative and overflowing limits.
- [ ] Handle empty results correctly.
- [ ] Reject unsupported syntax and trailing tokens.
- [ ] Remove redundant storage reads.
- [ ] Ensure query output never exposes password hashes.
- [ ] Add query parser tests.
- [ ] Add query execution tests.
- [ ] Add query authorization tests.
- [ ] Verify all query tests pass.

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
- [ ] Validate message and recipient data before writing.
- [ ] Define duplicate-recipient behavior.
- [ ] Implement and verify rollback for failed multi-record writes.
- [ ] Report rollback failure accurately.
- [ ] Fix ID generation across restarts.
- [ ] Prevent ID overflow and reuse.
- [ ] Synchronize concurrent ID allocation and writes.
- [ ] Ensure index changes occur only after successful storage changes.
- [ ] Add corruption and truncation tests.
- [ ] Add persistence and reopen tests.
- [ ] Add failed-write and rollback tests.
- [ ] Add concurrency tests.
- [ ] Run all storage tests using temporary directories.

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
- [ ] Validate JSON field types and constraints.
- [x] Standardize HTTP status codes and JSON errors.
- [x] Implement or verify registration.
- [x] Implement or verify login.
- [x] Implement or verify logout.
- [x] Implement or verify current-user lookup.
- [ ] Define inbox/sent/listing semantics.
- [x] Implement or verify authorized message listing.
- [x] Implement authorized single-message retrieval.
- [x] Implement validated message creation.
- [x] Implement deletion according to the documented policy.
- [x] Keep unsafe generic SQL disabled for ordinary users.
- [ ] Add real HTTP integration tests.
- [ ] Test server startup failure and shutdown.
- [ ] Test persistence across server restart.
- [ ] Run all API integration tests.

## G. CLI
- [x] Ensure CLI communicates over HTTP only.
- [x] Ensure CLI does not access storage files directly.
- [ ] Implement help.
- [ ] Implement status.
- [ ] Implement login.
- [ ] Implement logout.
- [ ] Implement whoami.
- [ ] Implement authorized SHOW TABLES.
- [ ] Implement authorized DESCRIBE.
- [ ] Implement the documented SELECT subset.
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
- [ ] Run API integration tests.
- [ ] Run CLI/server end-to-end tests.
- [x] Review test output for skipped or misleading tests.
- [x] Verify existing storage files were not overwritten or migrated.
- [x] Verify no tracked generated build artifacts remain.
- [x] Inspect the final diff for unrelated changes.
- [x] Confirm EMBER remains untouched.
- [x] Mark only genuinely verified TODO items complete.
- [ ] Document blocked items and known limitations.
- [x] Provide exact commands run and their actual outcomes.
- [x] Do not push or merge changes.
