# Frontend feature boundaries

- `services/`: typed HTTP transport and API-specific operations.
- `types/`: API wire contracts; UI models must not pretend the API returns fields it does not.
- `features/auth/`: authentication forms and session transitions (next extraction).
- `features/mailbox/`: folder selection, filtering, sorting and selection (next extraction).
- `features/compose/`: recipient validation and send flow (next extraction).
- `components/`: reusable visual elements and the existing POSTO animations.

The legacy `App.tsx` is still the screen composition root during this migration. New behavior should be added to feature modules rather than growing that component further.
