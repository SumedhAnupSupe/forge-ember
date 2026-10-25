from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from app.config import get_settings
from app.clients.forge_client import ForgeError, forge
from app.api import auth, mail

settings = get_settings()
app = FastAPI(title="EMBER Backend", version="0.1.0")
app.add_middleware(CORSMiddleware, allow_origins=settings.cors_origins,
                   allow_credentials=True, allow_methods=["GET", "POST", "PATCH", "DELETE", "OPTIONS"],
                   allow_headers=["Authorization", "Content-Type"])


@app.exception_handler(ForgeError)
async def forge_error_handler(_: Request, exc: ForgeError) -> JSONResponse:
    code = "FORGE_UNAVAILABLE" if exc.status_code >= 500 else "FORGE_REQUEST_FAILED"
    return JSONResponse(status_code=exc.status_code,
                        content={"success": False, "error": {"code": code, "message": str(exc)}})


@app.get("/health")
async def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/health/forge")
async def forge_health() -> dict[str, str]:
    await forge.request("GET", "/health")
    return {"status": "ok", "service": "forge"}


app.include_router(auth.router, prefix="/api/v1")
app.include_router(mail.router, prefix="/api/v1")
