import os
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    APP_NAME: str = "DomOS AI Gateway"
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    DEBUG: bool = True

    # AI API Keys
    OPENAI_API_KEY: str = os.getenv("OPENAI_API_KEY", "")
    ANTHROPIC_API_KEY: str = os.getenv("ANTHROPIC_API_KEY", "")
    GEMINI_API_KEY: str = os.getenv("GEMINI_API_KEY", "")

    # Ollama Local Fallback
    OLLAMA_BASE_URL: str = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
    DEFAULT_LLM_MODEL: str = os.getenv("DEFAULT_LLM_MODEL", "gpt-4o")

    # Audio Engine
    STT_ENGINE: str = os.getenv("STT_ENGINE", "whisper") # whisper | openai
    TTS_ENGINE: str = os.getenv("TTS_ENGINE", "edge-tts") # edge-tts | elevenlabs
    TTS_VOICE: str = os.getenv("TTS_VOICE", "en-US-ChristopherNeural")

    # MQTT Broker
    MQTT_BROKER_HOST: str = os.getenv("MQTT_BROKER_HOST", "localhost")
    MQTT_BROKER_PORT: int = int(os.getenv("MQTT_BROKER_PORT", "1883"))
    MQTT_CLIENT_ID: str = "domos-ai-gateway"

    class Config:
        env_file = ".env"

settings = Settings()
