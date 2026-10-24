from typing import Any
import httpx
from app.config import get_settings
from app.errors import api_error

class ForgeClient:
    """HTTP-only adapter; EMBER never reads FORGE's binary data files."""
    def __init__(self, base_url: str | None = None, timeout: float | None = None) -> None:
        settings = get_settings()
        self.base_url = (base_url or settings.forge_base_url).rstrip("/")
        self.timeout = timeout or settings.forge_timeout_seconds

    async def request(self, method: str, path: str, *, token: str | None = None,
                      json_body: dict[str, Any] | None = None,
                      params: dict[str, str] | None = None) -> tuple[int, Any]:
        headers = {"Accept": "application/json"}
        if token:
            headers["Authorization"] = f"Bearer {token}"
        try:
            async with httpx.AsyncClient(base_url=self.base_url, timeout=self.timeout) as client:
                response = await client.request(method, path, headers=headers, json=json_body, params=params)
        except httpx.TimeoutException as exc:
            raise api_error(504, "FORGE_TIMEOUT", "The database service did not respond in time.") from exc
        except httpx.RequestError as exc:
            raise api_error(503, "FORGE_UNAVAILABLE", "The database service is unavailable.") from exc
        try:
            payload = response.json() if response.content else {}
        except ValueError as exc:
            raise api_error(502, "INVALID_FORGE_RESPONSE", "The database service returned invalid JSON.") from exc
        if not isinstance(payload, (dict, list)):
            raise api_error(502, "INVALID_FORGE_RESPONSE", "The database service returned an unexpected response.")
        return response.status_code, payload

    async def require_success(self, method: str, path: str, *, token: str | None = None,
                              json_body: dict[str, Any] | None = None,
                              params: dict[str, str] | None = None,
                              accepted: tuple[int, ...] = (200, 201)) -> Any:
        status, payload = await self.request(method, path, token=token, json_body=json_body, params=params)
        if status in accepted:
            return payload
        message = payload.get("error", "The database request failed.") if isinstance(payload, dict) else "The database request failed."
        if status in (400, 409, 422):
            raise api_error(status, "FORGE_REQUEST_REJECTED", str(message))
        if status == 401:
            raise api_error(401, "UNAUTHORIZED", "Your session is invalid or expired. Please sign in again.")
        if status == 403:
            raise api_error(403, "FORBIDDEN", "You are not allowed to perform this action.")
        if status == 404:
            raise api_error(404, "NOT_FOUND", "The requested resource was not found.")
        if status >= 500:
            raise api_error(502, "FORGE_FAILURE", "The database service could not complete the request.")
        raise api_error(502, "FORGE_FAILURE", str(message))

forge = ForgeClient()
