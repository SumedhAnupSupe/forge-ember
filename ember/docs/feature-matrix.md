# EMBER feature matrix

| Feature | Upstream support | EMBER behavior in this stage |
|---|---|---|
| Register / login / logout | FORGE routes exist | Forward to FORGE |
| Current user | `GET /api/v1/users/me` | Forward |
| Inbox / sent messages | `GET /api/v1/messages` returns authorized messages with IDs and basic fields | Fetch and classify conservatively; missing sender/recipient metadata is not invented |
| Read detail | `GET /api/v1/messages/{id}` | Forward |
| Send | `POST /api/v1/messages` requires recipient user IDs | Forward numeric recipient IDs only |
| Search / sorting | Frontend can filter/sort loaded data | Client-side over returned fields only |
| Read / star / trash / restore | No per-user mailbox-state API yet | Not presented as persisted server features |
| Recipient email lookup | No authorized directory lookup endpoint yet | Requires FORGE API extension |
| Attachments | No message attachment API | Not implemented |
| Reply | Can send a new message after recipient resolution | UI integration depends on recipient metadata |
