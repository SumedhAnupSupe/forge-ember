from fastapi.testclient import TestClient
from app.main import app


def test_health_does_not_require_forge() -> None:
    response = TestClient(app).get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_openapi_exposes_auth_and_mail_contracts() -> None:
    schema = TestClient(app).get("/openapi.json").json()
    paths = schema["paths"]
    assert "/api/v1/auth/register" in paths
    assert "/api/v1/auth/login" in paths
    assert "/api/v1/mail/{folder}" in paths
    assert "/api/v1/mail" in paths
