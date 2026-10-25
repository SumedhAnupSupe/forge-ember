from fastapi import APIRouter, Header, HTTPException
from app.clients.forge_client import forge
from app.schemas import AuthRequest, AuthResponse, RegisterRequest, UserOut

router = APIRouter(prefix="/auth", tags=["authentication"])


async def user_from_token(token: str) -> UserOut:
    raw = await forge.me(token)
    user_id = int(raw["id"])
    rows = await forge.query(token, f"SELECT id, name, email, created_at FROM users WHERE id = {user_id} LIMIT 1")
    created_at = int(rows[0].get("created_at", 0)) if rows else 0
    return UserOut(id=user_id, name=raw["name"], email=raw["email"], created_at=created_at)


def require_token(authorization: str | None) -> str:
    if not authorization or not authorization.startswith("Bearer "):
        raise HTTPException(status_code=401, detail="Missing bearer token")
    return authorization[7:].strip()


@router.post("/register", response_model=AuthResponse, status_code=201)
async def register(payload: RegisterRequest) -> AuthResponse:
    await forge.register(payload.name.strip(), str(payload.email).lower(), payload.password)
    result = await forge.login(str(payload.email).lower(), payload.password)
    token = str(result["token"])
    return AuthResponse(user=await user_from_token(token), token=token)


@router.post("/login", response_model=AuthResponse)
async def login(payload: AuthRequest) -> AuthResponse:
    result = await forge.login(str(payload.email).lower(), payload.password)
    token = str(result["token"])
    return AuthResponse(user=await user_from_token(token), token=token)


@router.post("/logout")
async def logout(authorization: str | None = Header(default=None)) -> dict[str, bool]:
    await forge.logout(require_token(authorization))
    return {"success": True}


@router.get("/me", response_model=UserOut)
async def me(authorization: str | None = Header(default=None)) -> UserOut:
    return await user_from_token(require_token(authorization))
