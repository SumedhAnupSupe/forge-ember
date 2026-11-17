from __future__ import annotations

import pytest

from app.api.mail import enrich


USERS = {
    1: {"id": 1, "name": "Alice", "email": "alice@example.com"},
    2: {"id": 2, "name": "Bob", "email": "bob@example.com"},
    3: {"id": 3, "name": "Carol", "email": "carol@example.com"},
    4: {"id": 4, "name": "Dave", "email": "dave@example.com"},
    5: {"id": 5, "name": "Eve", "email": "eve@example.com"},
}
RECIPIENTS = [
    {"message_id": 42, "user_id": 2, "type": "TO"},
    {"message_id": 42, "user_id": 3, "type": "CC"},
    {"message_id": 42, "user_id": 4, "type": "BCC"},
    {"message_id": 42, "user_id": 5, "type": "BCC"},
]
MESSAGE = {
    "id": 42, "sender_id": 1, "subject": "Private envelope", "body": "Hello",
    "created_at": 1, "is_read": False, "is_starred": False, "is_deleted": False,
    "sender": USERS[1], "recipients": RECIPIENTS,
}


def test_sender_sees_all_recipients_including_bcc() -> None:
    message = enrich(MESSAGE, viewer_id=1)
    assert [recipient.id for recipient in message.recipients] == [2, 3, 4, 5]


def test_visible_recipient_cannot_see_other_bcc_identities() -> None:
    message = enrich(MESSAGE, viewer_id=2)
    assert [recipient.id for recipient in message.recipients] == [2, 3]
    assert all(recipient.id not in {4, 5} for recipient in message.recipients)


def test_bcc_recipient_sees_only_own_bcc_identity() -> None:
    message = enrich(MESSAGE, viewer_id=4)
    assert [recipient.id for recipient in message.recipients] == [2, 3, 4]
    assert 5 not in {recipient.id for recipient in message.recipients}
