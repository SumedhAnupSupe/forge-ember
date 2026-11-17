from __future__ import annotations

import asyncio

from app.api.mail import list_mail
from app.clients.forge_client import forge


def test_mailbox_search_sort_and_offset_pagination(monkeypatch) -> None:
    async def me(_token: str) -> dict:
        return {"id": 1, "name": "Alice", "email": "alice@example.com"}

    async def messages(_token: str, include_deleted: bool = False) -> list[dict]:
        return [
            {"id": 10, "sender_id": 1, "subject": "alpha first", "body": "one", "created_at": 30, "sender": {"id": 1, "name": "Alice", "email": "alice@example.com"}, "recipients": []},
            {"id": 11, "sender_id": 1, "subject": "beta", "body": "two", "created_at": 20, "sender": {"id": 1, "name": "Alice", "email": "alice@example.com"}, "recipients": []},
            {"id": 12, "sender_id": 1, "subject": "alpha second", "body": "three", "created_at": 10, "sender": {"id": 1, "name": "Alice", "email": "alice@example.com"}, "recipients": []},
        ]

    monkeypatch.setattr(forge, "me", me)
    monkeypatch.setattr(forge, "messages", messages)

    result = asyncio.run(list_mail(
        "sent", authorization="Bearer test", q="alpha", sort="newest", limit=1, offset=1
    ))
    assert len(result) == 1
    assert result[0].id == 12
    assert result[0].subject == "alpha second"


def test_mailbox_pagination_bounds_are_declared_in_openapi() -> None:
    from fastapi.testclient import TestClient
    from app.main import app

    parameters = TestClient(app).get("/openapi.json").json()["paths"]["/api/v1/mail/{folder}"]["get"]["parameters"]
    names = {parameter["name"] for parameter in parameters}
    assert {"q", "sort", "limit", "offset"}.issubset(names)
