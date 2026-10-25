from fastapi import APIRouter, Header, HTTPException, Query
from app.clients.forge_client import forge
from app.schemas import MessageOut, RecipientOut, SendMailRequest, SenderOut, StateUpdate

router = APIRouter(prefix="/mail", tags=["mail"])


def require_token(authorization: str | None) -> str:
    if not authorization or not authorization.startswith("Bearer "):
        raise HTTPException(status_code=401, detail="Missing bearer token")
    return authorization[7:].strip()


async def query_user(token: str, user_id: int) -> dict:
    rows = await forge.query(token, f"SELECT id, name, email, created_at FROM users WHERE id = {user_id} LIMIT 1")
    if not rows:
        raise HTTPException(status_code=502, detail=f"FORGE returned no user record for id {user_id}")
    return rows[0]


async def enrich(token: str, raw: dict) -> MessageOut:
    sender = await query_user(token, int(raw["sender_id"]))
    recs = await forge.query(token, f"SELECT message_id, user_id, type FROM recipients WHERE message_id = {int(raw['id'])} LIMIT 100")
    recipients = []
    for rec in recs:
        person = await query_user(token, int(rec["user_id"]))
        recipients.append(RecipientOut(id=int(person["id"]), name=str(person["name"]),
                                       email=str(person["email"]), type=str(rec.get("type", "TO")).upper()))
    return MessageOut(id=int(raw["id"]),
        sender=SenderOut(id=int(sender["id"]), name=str(sender["name"]), email=str(sender["email"])),
        recipients=recipients, subject=str(raw.get("subject", "")), body=str(raw.get("body", "")),
        created_at=int(raw.get("created_at", 0)), is_read=bool(raw.get("is_read", False)),
        is_starred=bool(raw.get("is_starred", False)), is_deleted=bool(raw.get("is_deleted", False)))


@router.get("/{folder}", response_model=list[MessageOut])
async def list_mail(folder: str, authorization: str | None = Header(default=None),
                    q: str | None = Query(default=None, max_length=200),
                    sort: str = Query(default="newest", pattern="^(newest|oldest)$")) -> list[MessageOut]:
    if folder not in {"inbox", "sent", "starred", "trash"}:
        raise HTTPException(status_code=404, detail="Unknown mailbox")
    token = require_token(authorization)
    me = await forge.me(token)
    rows = await forge.messages(token, include_deleted=(folder == "trash"))
    result = []
    for row in rows:
        message = await enrich(token, row)
        sender = message.sender.id == int(me["id"])
        recipient = any(r.id == int(me["id"]) for r in message.recipients)
        if folder == "inbox" and (sender or not recipient or message.is_deleted): continue
        if folder == "sent" and not sender: continue
        if folder == "starred" and not message.is_starred: continue
        if folder == "trash" and not message.is_deleted: continue
        if q:
            haystack = " ".join([message.subject, message.body, message.sender.name, message.sender.email,
                                 *(r.name + " " + r.email for r in message.recipients)]).casefold()
            if q.casefold() not in haystack: continue
        result.append(message)
    result.sort(key=lambda item: item.created_at, reverse=sort == "newest")
    return result


@router.get("/item/{message_id}", response_model=MessageOut)
async def get_mail(message_id: int, authorization: str | None = Header(default=None)) -> MessageOut:
    token = require_token(authorization)
    me = await forge.me(token)
    message = await enrich(token, await forge.message(token, message_id))
    if message.sender.id != int(me["id"]) and all(r.id != int(me["id"]) for r in message.recipients):
        raise HTTPException(status_code=404, detail="Message not found")
    return message


@router.post("", status_code=201)
async def send_mail(payload: SendMailRequest, authorization: str | None = Header(default=None)) -> dict:
    token = require_token(authorization)
    emails = [str(r.email).lower() for r in payload.recipients]
    if len(set(emails)) != len(emails):
        raise HTTPException(status_code=422, detail="Duplicate recipient email")
    recipients = []
    for item, email in zip(payload.recipients, emails):
        try:
            match = await forge.lookup_user(token, email)
        except Exception as exc:
            from app.clients.forge_client import ForgeError
            if isinstance(exc, ForgeError) and exc.status_code == 404:
                raise HTTPException(status_code=422, detail=f"Recipient account not found: {email}") from exc
            raise
        recipients.append({"user_id": int(match["id"]), "type": item.type})
    result = await forge.send(token, payload.subject, payload.body, recipients)
    return {"success": True, "id": int(result["id"])}


@router.patch("/{message_id}/read")
async def mark_read(message_id: int, payload: StateUpdate, authorization: str | None = Header(default=None)) -> dict:
    await forge.request("PATCH", f"/api/v1/messages/{message_id}/state", token=require_token(authorization),
                        json={"is_read": payload.value})
    return {"success": True}


@router.patch("/{message_id}/star")
async def mark_star(message_id: int, payload: StateUpdate, authorization: str | None = Header(default=None)) -> dict:
    await forge.request("PATCH", f"/api/v1/messages/{message_id}/state", token=require_token(authorization),
                        json={"is_starred": payload.value})
    return {"success": True}


@router.delete("/{message_id}")
async def trash(message_id: int, authorization: str | None = Header(default=None)) -> dict:
    await forge.request("PATCH", f"/api/v1/messages/{message_id}/mailbox", token=require_token(authorization),
                        json={"is_deleted": True})
    return {"success": True}


@router.post("/{message_id}/restore")
async def restore(message_id: int, authorization: str | None = Header(default=None)) -> dict:
    await forge.request("PATCH", f"/api/v1/messages/{message_id}/mailbox", token=require_token(authorization),
                        json={"is_deleted": False})
    return {"success": True}


@router.delete("/{message_id}/permanent")
async def permanent_delete(message_id: int, authorization: str | None = Header(default=None)) -> dict:
    await forge.request("DELETE", f"/api/v1/messages/{message_id}/mailbox", token=require_token(authorization))
    return {"success": True}
