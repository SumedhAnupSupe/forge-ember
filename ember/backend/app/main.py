from typing import Any
from fastapi import Depends, FastAPI, Header, HTTPException, Request
from fastapi.exceptions import RequestValidationError
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from app.config import get_settings
from app.errors import api_error
from app.forge_client import forge
from app.schemas import LoginOut, LoginRequest, MessageCreated, MessageOut, RegisterRequest, SendMessageRequest, UserOut

settings = get_settings()
app = FastAPI(title="EMBER API", description="POSTO application API backed by FORGE.", version="0.1.0")
app.add_middleware(CORSMiddleware, allow_origins=settings.cors_origins, allow_credentials=False,
                   allow_methods=["GET", "POST", "DELETE", "OPTIONS"],
                   allow_headers=["Authorization", "Content-Type"])

@app.exception_handler(HTTPException)
async def http_error_handler(_: Request, exc: HTTPException) -> JSONResponse:
    detail = exc.detail
    error = detail if isinstance(detail, dict) and "code" in detail else {"code": "REQUEST_FAILED", "message": str(detail)}
    return JSONResponse(status_code=exc.status_code, content={"error": error}, headers=exc.headers)

@app.exception_handler(RequestValidationError)
async def validation_error_handler(_: Request, exc: RequestValidationError) -> JSONResponse:
    fields = sorted({str(item.get("loc", ["request"])[-1]) for item in exc.errors()})
    return JSONResponse(status_code=422, content={"error": {"code": "VALIDATION_ERROR", "message": "Invalid request fields: " + ", ".join(fields)}})

def bearer_token(authorization: str | None = Header(default=None)) -> str:
    if not authorization or not authorization.startswith("Bearer ") or not authorization[7:].strip():
        raise api_error(401, "UNAUTHORIZED", "Sign in to continue.")
    return authorization[7:].strip()

def user_from_payload(payload: dict[str, Any]) -> UserOut:
    try:
        return UserOut(id=int(payload["id"]), name=str(payload["name"]), email=str(payload["email"]),
                       created_at=int(payload.get("created_at", 0)))
    except (KeyError, TypeError, ValueError) as exc:
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an incomplete user record.") from exc

@app.get("/health")
@app.get("/health/live")
async def health() -> dict[str, str]:
    return {"status": "ok"}

@app.get("/health/ready")
async def readiness() -> dict[str, str]:
    status, payload = await forge.request("GET", "/health")
    if status != 200 or not isinstance(payload, dict) or payload.get("status") != "ok":
        raise api_error(503, "NOT_READY", "The database service is not ready.")
    return {"status": "ready", "forge": "ok"}

@app.get("/api/v1/system/status")
async def system_status() -> dict[str, str]:
    status, payload = await forge.request("GET", "/health")
    ok = status == 200 and isinstance(payload, dict) and payload.get("status") == "ok"
    return {"status": "ok" if ok else "degraded", "forge": "ok" if ok else "unavailable"}

@app.post("/api/v1/auth/register", response_model=UserOut, status_code=201)
@app.post("/api/v1/users", response_model=UserOut, status_code=201, include_in_schema=False)
async def register(data: RegisterRequest) -> UserOut:
    payload = await forge.require_success("POST", "/api/v1/users",
        json_body={"name": data.name, "email": str(data.email).lower(), "password": data.password}, accepted=(201,))
    if not isinstance(payload, dict):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an incomplete user record.")
    return user_from_payload(payload)

@app.post("/api/v1/auth/login", response_model=LoginOut)
async def login(data: LoginRequest) -> LoginOut:
    payload = await forge.require_success("POST", "/api/v1/auth/login",
        json_body={"email": str(data.email).lower(), "password": data.password}, accepted=(200,))
    if not isinstance(payload, dict) or not isinstance(payload.get("token"), str):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid login response.")
    user_payload = await forge.require_success("GET", "/api/v1/users/me", token=payload["token"])
    if not isinstance(user_payload, dict):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid user record.")
    return LoginOut(token=payload["token"], user=user_from_payload(user_payload))

@app.post("/api/v1/auth/logout")
async def logout(token: str = Depends(bearer_token)) -> dict[str, str]:
    await forge.require_success("POST", "/api/v1/auth/logout", token=token)
    return {"status": "ok"}

@app.get("/api/v1/auth/me", response_model=UserOut)
@app.get("/api/v1/users/me", response_model=UserOut, include_in_schema=False)
async def me(token: str = Depends(bearer_token)) -> UserOut:
    payload = await forge.require_success("GET", "/api/v1/users/me", token=token)
    if not isinstance(payload, dict):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid user record.")
    return user_from_payload(payload)

async def enriched_message(item: dict[str, Any], token: str, current_user_id: int) -> dict[str, Any]:
    try:
        sender_id = int(item["sender_id"])
        message_id = int(item["id"])
        sender = await forge.require_success("GET", f"/api/v1/users/{sender_id}", token=token)
        recipients = await forge.require_success("GET", f"/api/v1/messages/{message_id}/recipients", token=token)
        if not isinstance(sender, dict) or not isinstance(recipients, list):
            raise ValueError("invalid enrichment response")
        return {
            "id": message_id, "sender_id": sender_id,
            "sender": {"id": int(sender["id"]), "name": str(sender["name"]), "email": str(sender["email"])},
            "recipients": recipients,
            "subject": str(item["subject"]), "body": str(item["body"]),
            "created_at": int(item["created_at"]),
            # FORGE currently has no per-user mailbox-state API. Do not persist fake state.
            "is_read": sender_id == current_user_id,
            "is_starred": False, "is_deleted": False,
        }
    except HTTPException:
        raise
    except (KeyError, TypeError, ValueError) as exc:
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an incomplete message record.") from exc

@app.get("/api/v1/users/lookup")
async def user_lookup(email: str, token: str = Depends(bearer_token)) -> dict[str, Any]:
    payload = await forge.require_success("GET", "/api/v1/users/lookup", token=token, params={"email": email})
    if not isinstance(payload, dict):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid user record.")
    return payload

async def get_all_messages(token: str) -> tuple[int, list[dict[str, Any]]]:
    user_payload = await forge.require_success("GET", "/api/v1/users/me", token=token)
    if not isinstance(user_payload, dict) or "id" not in user_payload:
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid user record.")
    current_user_id = int(user_payload["id"])
    payload = await forge.require_success("GET", "/api/v1/messages", token=token)
    if not isinstance(payload, list):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid message list.")
    enriched = await __import__("asyncio").gather(
        *(enriched_message(item, token, current_user_id) for item in payload if isinstance(item, dict))
    )
    return current_user_id, enriched

@app.get("/api/v1/messages")
async def inbox(token: str = Depends(bearer_token)) -> list[dict[str, Any]]:
    _, messages = await get_all_messages(token)
    return messages

@app.get("/api/v1/mailboxes/inbox")
async def inbox_only(token: str = Depends(bearer_token)) -> list[dict[str, Any]]:
    user_id, messages = await get_all_messages(token)
    return [m for m in messages if any(int(r.get("id", 0)) == user_id for r in m["recipients"])]

@app.get("/api/v1/mailboxes/sent")
async def sent_only(token: str = Depends(bearer_token)) -> list[dict[str, Any]]:
    user_id, messages = await get_all_messages(token)
    return [m for m in messages if m["sender_id"] == user_id]

@app.get("/api/v1/messages/{message_id}", response_model=MessageOut)
async def message_detail(message_id: int, token: str = Depends(bearer_token)) -> dict[str, Any]:
    payload = await forge.require_success("GET", f"/api/v1/messages/{message_id}", token=token)
    if not isinstance(payload, dict):
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database returned an invalid message record.")
    return await enriched_message(payload, token, int((await forge.require_success("GET", "/api/v1/users/me", token=token))["id"]))

@app.post("/api/v1/messages", response_model=MessageCreated, status_code=201)
async def send_message(data: SendMessageRequest, token: str = Depends(bearer_token)) -> MessageCreated:
    if len({recipient.user_id for recipient in data.recipients}) != len(data.recipients):
        raise api_error(422, "DUPLICATE_RECIPIENT", "Each recipient may appear only once.")
    payload = await forge.require_success("POST", "/api/v1/messages", token=token,
        json_body={"subject": data.subject, "body": data.body,
                   "recipients": [recipient.model_dump() for recipient in data.recipients]}, accepted=(201,))
    try:
        return MessageCreated(id=int(payload["id"]))
    except (KeyError, TypeError, ValueError) as exc:
        raise api_error(502, "INVALID_FORGE_RESPONSE", "The database did not confirm the sent message.") from exc

@app.delete("/api/v1/messages/{message_id}")
async def delete_message(message_id: int, token: str = Depends(bearer_token)) -> dict[str, str]:
    await forge.require_success("DELETE", f"/api/v1/messages/{message_id}", token=token)
    return {"status": "ok"}
