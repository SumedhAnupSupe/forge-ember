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
- `message.txn`: temporary write-ahead journal for a message and its complete recipient envelope; removed after commit/replay.
- `purge.txn`: write-ahead journal for physical compaction after the final mailbox copy is removed.

## Storage compatibility

Manifest version 1 recorded the original three record sizes. Version 2 adds the mailbox-state record size. On startup, version 1 and manifest-less stores are validated and upgraded: each existing message gets a state row for its sender and each active recipient, initialized from the legacy shared flags. The migration is idempotent by `(message_id, user_id)` and only tags the store as version 2 after migration succeeds. An unknown version, magic value, or record-size layout is rejected.

The legacy-adoption path is not a general schema migration framework. Future record changes require a new version and an explicit migration strategy.

## Message transaction recovery

Message creation writes a versioned `message.txn` journal containing the encoded message and all recipient records, with a checksum, before appending either data file. Startup validates and replays a pending journal idempotently. If the final fixed-size record append was interrupted, recovery truncates only the incomplete tail and writes the complete record from the journal. The sender and active recipients' mailbox-state rows are then created; startup migration repairs missing rows if the process stops before that step. Both journals use stream flush and same-directory atomic rename; they do not currently issue OS-level `fsync`, so they are not guarantees against sudden power loss or storage-controller cache loss.

## Indexes

FORGE uses a separate-chaining hash table with FNV-1a 32-bit hashing. Indexes are in-memory and rebuilt from persisted records at server startup. Hash-table collisions are resolved by linked lists in each bucket.

## Current limitations

- New and migrated records use independent mailbox-state rows. Message still contains legacy flags for compatibility; new service operations read/write the per-user row.
- Permanent removal deletes the caller's mailbox-state row. When no participant state remains, `purge.txn` drives idempotent compaction of the message, recipient, and mailbox-state files.
- The journals cover message/recipient creation and final-participant purge, not every state update or every storage mutation.
- The engine does not provide general SQL transactions, joins, or arbitrary write queries.
