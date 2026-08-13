import edge_tts
import logging
import asyncio
import io

logger = logging.getLogger("domos.tts")

class TTSService:
    def __init__(self, voice: str = "en-US-ChristopherNeural"):
        self.voice = voice

    async def generate_speech_bytes(self, text: str, voice: str = "") -> bytes:
        """Convert text to MP3/PCM audio bytes using Edge-TTS."""
        selected_voice = voice or self.voice
        try:
            communicate = edge_tts.Communicate(text, selected_voice)
            audio_buffer = io.BytesIO()
            async for chunk in communicate.stream():
                if chunk["type"] == "audio":
                    audio_buffer.write(chunk["data"])
            return audio_buffer.getvalue()
        except Exception as e:
            logger.error(f"TTS generation error: {e}")
            return b""

tts_service = TTSService()
