from pathlib import Path

from pydantic_settings import BaseSettings, SettingsConfigDict


BACKEND_DIR = Path(__file__).resolve().parent


class Settings(BaseSettings):
    # The shared root .env contains deployment secrets. backend-python/.env may
    # override non-secret developer settings when the service is run directly.
    model_config = SettingsConfigDict(
        env_file=(BACKEND_DIR.parent / ".env", BACKEND_DIR / ".env"),
        extra="ignore",
    )

    APP_NAME: str = "DomOS OpenRouter Voice Gateway"
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    # Namespaced to avoid collisions with generic DEBUG variables injected by
    # shells, IDEs and package managers.
    DOMOS_DEBUG: bool = False

    OPENROUTER_API_KEY: str = ""
    OPENROUTER_BASE_URL: str = "https://openrouter.ai/api/v1"
    OPENROUTER_MODEL: str = "openrouter/free"
    OPENROUTER_AUDIO_MODEL: str = (
        "nvidia/nemotron-3-nano-omni-30b-a3b-reasoning:free"
    )
    OPENROUTER_TIMEOUT_SEC: float = 60.0
    OPENROUTER_HTTP_REFERER: str = "http://localhost:3000"
    STT_PROVIDER: str = "google-web"
    STT_LANGUAGE: str = "vi-VN"
    TTS_PROVIDER: str = "google"
    TTS_VOICE: str = "vi-VN-HoaiMyNeural"
    TTS_TIMEOUT_SEC: float = 15.0
    CONVERSATION_DB_PATH: str = "data/conversations.db"

    # Deployment addresses come from the shared root .env.
    MQTT_BROKER_HOST: str = "localhost"
    MQTT_BROKER_PORT: int = 1883
    MQTT_CLIENT_ID: str = "domos-ai-gateway"
    MQTT_USERNAME: str = ""
    MQTT_PASSWORD: str = ""

    # Voice Protocol v3: PCM, 16kHz, mono, 60ms frames
    VOICE_SESSION_TIMEOUT_SEC: int = 30
    VOICE_AUTH_TOKEN: str = ""


settings = Settings()
