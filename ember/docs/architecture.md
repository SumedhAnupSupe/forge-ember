# EMBER architecture decisions

- **FORGE owns data and authentication.** EMBER is an adapter and application layer; no direct storage-file access.
- **POSTO is the visual foundation, not the application architecture.** Keep its warm stationery palette, typography, Logo, PostalAnimation and SendFlightAnimation while moving network, auth, mailbox and compose responsibilities into focused modules.
- **No mock production data.** Sample messages and random users are for design previews only and must not be returned by the live API service.
- **No false success states.** A send animation may begin on submission, but the success toast and clearing of the compose form follow a successful FORGE response.
- **Mailbox state is per user.** Read/star/trash flags cannot be stored globally on a message because one recipient's action must not affect other recipients. This needs a FORGE schema/API extension before implementation.
- **API adapter only.** EMBER forwards supported routes and normalizes errors; it must not bypass the FORGE REST API.
- **Token handling is currently development-grade.** Browser bearer storage is not the final production design; use an HttpOnly session cookie and CSRF protection before deployment.
