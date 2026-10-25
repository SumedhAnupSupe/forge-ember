from functools import lru_cache
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    forge_base_url: str = "http://127.0.0.1:8080"
    frontend_origins: str = "http://localhost:3000,http://127.0.0.1:3000"
    request_timeout_seconds: float = 5.0
    model_config = SettingsConfigDict(env_file=".env", extra="ignore")

    @property
    def cors_origins(self) -> list[str]:
        return [item.strip() for item in self.frontend_origins.split(",") if item.strip()]


@lru_cache
def get_settings() -> Settings:
    return Settings()
