# Database Design

FORGE is a compact educational database engine, not a general-purpose SQL database.

## Current records

| Record | Fields | Encoded size |
|---|---|---:|
| User | id, name[64], email[128], password_hash[256], created_at, is_deleted | 461 bytes |
| Message | id, sender_id, subject[128], body[512], created_at, is_read, is_starred, is_deleted | 659 bytes |
| Recipient | message_id, user_id, type[4], is_deleted | 13 bytes |
| MailboxState | message_id, user_id, is_read, is_starred, is_deleted | 11 bytes |

Records are serialized field-by-field in little-endian numeric format. The implementation does not write raw C++ structs to disk.

## Files

- `users.dat`: fixed-width user records.
- `messages.dat`: fixed-width message records.
- `recipients.dat`: fixed-width recipient records.
- `mailbox_states.dat`: fixed-width mailbox state records keyed by message and user.
- `format.meta`: storage manifest containing magic, format version, and expected record sizes.

## Storage compatibility

Manifest version 1 recorded the original three record sizes. Version 2 adds the mailbox-state record size. On startup, version 1 and manifest-less stores are validated and upgraded: each existing message gets a state row for its sender and each active recipient, initialized from the legacy shared flags. The migration is idempotent by `(message_id, user_id)` and only tags the store as version 2 after migration succeeds. An unknown version, magic value, or record-size layout is rejected.

The legacy-adoption path is not a general schema migration framework. Future record changes require a new version and an explicit migration strategy.

## Indexes

FORGE uses a separate-chaining hash table with FNV-1a 32-bit hashing. Indexes are in-memory and rebuilt from persisted records at server startup. Hash-table collisions are resolved by linked lists in each bucket.

## Current limitations

- New and migrated records use independent mailbox-state rows. Message still contains legacy flags for compatibility; new service operations read/write the per-user row.
- Permanent removal currently removes the participant's mailbox-state row. Shared message/recipient record compaction after the last participant removes their copy is still outstanding.
- Message plus recipient creation uses best-effort rollback; there is no crash-safe WAL yet.
- The engine does not provide general SQL transactions, joins, or arbitrary write queries.
