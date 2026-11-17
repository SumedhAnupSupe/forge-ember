# Database Design

FORGE is a compact educational database engine, not a general-purpose SQL database.

## Current records

| Record | Fields | Encoded size |
|---|---|---:|
| User | id, name[64], email[128], password_hash[256], created_at, is_deleted | 461 bytes |
| Message | id, sender_id, subject[128], body[512], created_at, is_read, is_starred, is_deleted | 659 bytes |
| Recipient | message_id, user_id, type[4], is_deleted | 13 bytes |

Records are serialized field-by-field in little-endian numeric format. The implementation does not write raw C++ structs to disk.

## Files

- `users.dat`: fixed-width user records.
- `messages.dat`: fixed-width message records.
- `recipients.dat`: fixed-width recipient records.
- `format.meta`: storage manifest containing magic, format version, and expected record sizes.

## Storage compatibility

Manifest version 1 is added to existing manifest-less directories only after the existing record files have passed record-size and decode validation. The manifest records the three encoded record sizes. An unknown version, magic value, or record-size layout is rejected without intentionally rewriting existing record files.

The legacy-adoption path is not a general schema migration framework. Future record changes require a new version and an explicit migration strategy.

## Indexes

FORGE uses a separate-chaining hash table with FNV-1a 32-bit hashing. Indexes are in-memory and rebuilt from persisted records at server startup. Hash-table collisions are resolved by linked lists in each bucket.

## Current limitations

- `is_read`, `is_starred`, and `is_deleted` live on Message, not on a per-user mailbox record. Actions are shared across all participants.
- Message deletion is currently soft deletion.
- Message plus recipient creation uses best-effort rollback; there is no crash-safe WAL yet.
- The engine does not provide general SQL transactions, joins, or arbitrary write queries.
