from __future__ import annotations
from typing import Any
import httpx
from app.config import get_settings


class ForgeError(Exception):
    def __init__(self, message: str, status_code: int = 502):
        super().__init__(message)
        self.status_code = status_code


class ForgeClient:
    def __init__(self) -> None:
        settings = get_settings()
        self.base_url = settings.forge_base_url.rstrip("/")
        self.timeout = settings.request_timeout_seconds

    async def request(self, method: str, path: str, *, token: str | None = None,
                      json: dict[str, Any] | None = None,
                      params: dict[str, Any] | None = None) -> Any:
        headers = {"Authorization": f"Bearer {token}"} if token else {}
        try:
            async with httpx.AsyncClient(timeout=self.timeout) as client:
                response = await client.request(method, self.base_url + path, headers=headers,
                                                json=json, params=params)
        except httpx.TimeoutException as exc:
            raise ForgeError("FORGE request timed out", 504) from exc
        except httpx.HTTPError as exc:
            raise ForgeError("FORGE service is unavailable", 503) from exc
        if response.status_code >= 400:
            try:
                data = response.json()
                message = data.get("error", data.get("message", "FORGE request failed"))
            except ValueError:
                message = "FORGE request failed"
            status = response.status_code if response.status_code < 500 else 502
            raise ForgeError(str(message), status)
        if not response.content:
            return None
        try:
            return response.json()
        except ValueError as exc:
            raise ForgeError("FORGE returned invalid JSON") from exc

    async def register(self, name: str, email: str, password: str) -> dict[str, Any]:
        return await self.request("POST", "/api/v1/users",
                                  json={"name": name, "email": email, "password": password})

    async def login(self, email: str, password: str) -> dict[str, Any]:
        return await self.request("POST", "/api/v1/auth/login",
                                  json={"email": email, "password": password})

    async def me(self, token: str) -> dict[str, Any]:
        return await self.request("GET", "/api/v1/users/me", token=token)

    async def logout(self, token: str) -> None:
        await self.request("POST", "/api/v1/auth/logout", token=token)

    async def messages(self, token: str, include_deleted: bool = False) -> list[dict[str, Any]]:
        data = await self.request("GET", "/api/v1/messages", token=token, params={"include_deleted": "true"} if include_deleted else None)
        if not isinstance(data, list):
            raise ForgeError("Unexpected FORGE message list response")
        return data

    async def message(self, token: str, message_id: int) -> dict[str, Any]:
        return await self.request("GET", f"/api/v1/messages/{message_id}", token=token)

    async def send(self, token: str, subject: str, body: str,
                   recipients: list[dict[str, Any]]) -> dict[str, Any]:
        return await self.request("POST", "/api/v1/messages", token=token,
                                  json={"subject": subject, "body": body, "recipients": recipients})

    async def query(self, token: str, sql: str) -> list[dict[str, Any]]:
        data = await self.request("POST", "/api/v1/query", token=token, json={"query": sql})
        if not isinstance(data, dict) or not isinstance(data.get("rows"), list):
            raise ForgeError("Unexpected FORGE query response")
        return data["rows"]


forge = ForgeClient()
