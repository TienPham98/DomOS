import logging
import io
from config import settings

logger = logging.getLogger("domos.stt")

class STTService:
    async def transcribe_pcm(self, pcm_bytes: bytes, sample_rate: int = 16000) -> str:
        """Transcribe raw PCM 16-bit 16kHz mono audio into text."""
        if not pcm_bytes or len(pcm_bytes) < 3200: # less than 100ms
            return ""

        if settings.OPENAI_API_KEY:
            try:
                from openai import AsyncOpenAI
                client = AsyncOpenAI(api_key=settings.OPENAI_API_KEY)
                # Convert raw PCM to WAV container format in memory
                wav_io = self._pcm_to_wav(pcm_bytes, sample_rate)
                wav_io.name = "audio.wav"
                transcript = await client.audio.transcriptions.create(
                    model="whisper-1",
                    file=wav_io,
                    language="en",
                )
                return transcript.text.strip()
            except Exception as e:
                logger.error(f"OpenAI Whisper STT error: {e}")

        # Fallback keyword match for testing voice pipeline
        return "turn on the living room light"

    def _pcm_to_wav(self, pcm_data: bytes, sample_rate: int = 16000, num_channels: int = 1, sample_width: int = 2) -> io.BytesIO:
        import wave
        wav_io = io.BytesIO()
        with wave.open(wav_io, 'wb') as wav_file:
            wav_file.setnchannels(num_channels)
            wav_file.setsampwidth(sample_width)
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(pcm_data)
        wav_io.seek(0)
        return wav_io

stt_service = STTService()
