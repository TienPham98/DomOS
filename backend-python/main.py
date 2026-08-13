import logging
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List, Optional

from config import settings
from services.llm_service import llm_service
from services.stt_service import stt_service
from services.tts_service import tts_service

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s")
logger = logging.getLogger("domos.main")

app = FastAPI(
    title=settings.APP_NAME,
    version="0.2.1",
    description="DomOS Python AI Gateway — STT, TTS, LLM Router & Smart Home Voice Control",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class ChatMessage(BaseModel):
    role: str
    content: str

class ChatRequest(BaseModel):
    messages: List[ChatMessage]
    provider: Optional[str] = "openai"
    model: Optional[str] = "gpt-4o"

class ChatResponse(BaseModel):
    text: str
    provider: str
    model: str
    audio_url: Optional[str] = None

@app.get("/health")
async def health_check():
    return {
        "status": "online",
        "service": settings.APP_NAME,
        "version": "0.2.1",
        "default_model": settings.DEFAULT_LLM_MODEL,
        "stt_engine": settings.STT_ENGINE,
        "tts_engine": settings.TTS_ENGINE,
    }

@app.post("/api/v1/chat", response_model=ChatResponse)
async def chat_endpoint(req: ChatRequest):
    messages_dict = [{"role": m.role, "content": m.content} for m in req.messages]
    result = await llm_service.chat_completion(
        messages=messages_dict,
        provider=req.provider or "openai",
        model=req.model or "gpt-4o",
    )
    return ChatResponse(
        text=result["text"],
        provider=result.provider if hasattr(result, "provider") else result.get("provider", "unknown"),
        model=result.model if hasattr(result, "model") else result.get("model", "unknown"),
    )

@app.websocket("/api/v1/voice/stream")
async def voice_stream_websocket(websocket: WebSocket):
    """Real-time voice stream endpoint for ESP32-S3 terminals."""
    await websocket.accept()
    logger.info("ESP32 voice terminal connected to audio stream.")
    pcm_buffer = bytearray()

    try:
        while True:
            data = await websocket.receive_bytes()
            # If received special command signal (e.g. 0xFE00 = END OF SPEECH)
            if data == b"END_OF_SPEECH":
                logger.info(f"Processing accumulated audio: {len(pcm_buffer)} bytes")
                # 1. Speech-To-Text
                transcript = await stt_service.transcribe_pcm(bytes(pcm_buffer))
                logger.info(f"Transcript: '{transcript}'")

                if transcript:
                    # 2. LLM response
                    llm_res = await llm_service.chat_completion(
                        messages=[{"role": "user", "content": transcript}]
                    )
                    response_text = llm_res["text"]

                    # 3. Text-To-Speech
                    audio_bytes = await tts_service.generate_speech_bytes(response_text)

                    # 4. Send text JSON then binary audio payload back to ESP32
                    await websocket.send_json({"transcript": transcript, "response": response_text})
                    await websocket.send_bytes(audio_bytes)

                pcm_buffer.clear()
            else:
                # Accumulate PCM 16kHz audio chunks
                pcm_buffer.extend(data)

    except WebSocketDisconnect:
        logger.info("ESP32 voice terminal disconnected.")
    except Exception as e:
        logger.error(f"Voice stream error: {e}")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host=settings.HOST, port=settings.PORT)
