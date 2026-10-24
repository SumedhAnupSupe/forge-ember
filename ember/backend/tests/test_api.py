from fastapi.testclient import TestClient
from app.main import app

def test_health_does_not_require_forge() -> None:
    with TestClient(app) as client:
        response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}

def test_protected_route_requires_bearer_token() -> None:
    with TestClient(app) as client:
        response = client.get("/api/v1/messages")
    assert response.status_code == 401
    assert response.json()["error"]["code"] == "UNAUTHORIZED"

def test_register_rejects_short_password_before_calling_forge() -> None:
    with TestClient(app) as client:
        response = client.post("/api/v1/auth/register", json={"name": "Ada", "email": "ada@example.test", "password": "short"})
    assert response.status_code == 422
    assert response.json()["error"]["code"] == "VALIDATION_ERROR"

def test_send_rejects_duplicate_recipients_before_calling_forge() -> None:
    with TestClient(app) as client:
        response = client.post("/api/v1/messages", headers={"Authorization": "Bearer test-token"},
            json={"subject": "Hello", "body": "World", "recipients": [{"user_id": 2, "type": "TO"}, {"user_id": 2, "type": "CC"}]})
    assert response.status_code == 422
    assert response.json()["error"]["code"] == "DUPLICATE_RECIPIENT"
