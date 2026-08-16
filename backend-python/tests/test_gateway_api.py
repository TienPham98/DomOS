import asyncio
import json
import tempfile
import unittest
import warnings
from array import array
from pathlib import Path
from unittest.mock import AsyncMock, patch

from starlette.exceptions import StarletteDeprecationWarning

warnings.filterwarnings("ignore", category=StarletteDeprecationWarning)

from fastapi.testclient import TestClient  # noqa: E402

import main
from services.conversation_store import ConversationStore
from services.openrouter_voice_service import (
    PCM_FRAME_BYTES,
    VoiceRegistry,
    VoiceSession,
    validate_dom_hello,
)


class FakeWebSocket:
    def __init__(self) -> None:
        self.text_messages: list[dict] = []
        self.binary_messages: list[bytes] = []

    async def send_text(self, payload: str) -> None:
        self.text_messages.append(json.loads(payload))

    async def send_bytes(self, payload: bytes) -> None:
        self.binary_messages.append(payload)


class GatewayApiTests(unittest.TestCase):
    def test_health_reports_cloud_only_voice_stack(self):
        with TestClient(main.app) as client:
            response = client.get("/health")

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assertEqual(payload["provider"], "openrouter")
        self.assertFalse(payload["local_ai"])
        self.assertEqual(payload["memory"], "sqlite")
        self.assertIn("stt_provider", payload)
        self.assertIn("tts_provider", payload)

    def test_conversation_endpoint_filters_by_device(self):
        with tempfile.TemporaryDirectory() as directory:
            store = ConversationStore(str(Path(directory) / "api.db"))
            asyncio.run(store.initialize())
            first = asyncio.run(store.create_turn("board-a", "xin chào", "openrouter", "free"))
            asyncio.run(store.complete_turn(first, "Chào bạn!"))
            second = asyncio.run(store.create_turn("board-b", "mấy giờ", "openrouter", "free"))
            asyncio.run(store.complete_turn(second, "Bây giờ là 20 giờ."))

            with patch.object(main, "conversation_store", store):
                with TestClient(main.app) as client:
                    response = client.get(
                        "/api/v1/conversations", params={"device_id": "board-a", "limit": 10}
                    )

        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assertEqual(payload["count"], 1)
        self.assertEqual(payload["items"][0]["device_id"], "board-a")

    def test_wallpaper_proxy_rejects_path_traversal(self):
        with TestClient(main.app) as client:
            response = client.get("/uploads/wallpapers/%2e%2e%2fsecret.jpg")
        # Starlette may reject the normalized path before it reaches the
        # endpoint (404); the endpoint itself rejects an unsafe filename (400).
        self.assertIn(response.status_code, {400, 404})

    def test_protocol_rejects_wrong_codec_or_version(self):
        invalid_messages = [
            {"type": "hello", "version": 2, "audio_params": {}},
            {
                "type": "hello",
                "version": 3,
                "audio_params": {
                    "codec": "opus",
                    "sample_rate": 16000,
                    "channels": 1,
                    "frame_duration": 60,
                },
            },
        ]
        for message in invalid_messages:
            with self.subTest(message=message), self.assertRaises(ValueError):
                validate_dom_hello(message)


class VoiceSessionTests(unittest.IsolatedAsyncioTestCase):
    async def test_registry_add_remove_is_idempotent(self):
        registry = VoiceRegistry()
        await registry.add("session-1")
        await registry.add("session-1")
        self.assertEqual(registry.count, 1)
        await registry.remove("session-1")
        self.assertEqual(registry.count, 0)

    async def test_listening_and_wake_states_only_emit_status_messages(self):
        websocket = FakeWebSocket()
        session = VoiceSession(websocket, "board", "session")

        await session.set_listening(notify_board=True)
        self.assertEqual(session.state, "LISTENING")
        self.assertEqual(websocket.text_messages[0]["state"], "start")
        self.assertEqual(websocket.text_messages[1]["emotion"], "listening")

        await session.set_wake_word(notify_board=True)
        self.assertEqual(session.state, "WAKE_WORD")
        self.assertEqual(websocket.text_messages[-2]["state"], "wake")
        self.assertEqual(websocket.text_messages[-1]["emotion"], "idle")

    async def test_vad_ignores_silence_then_starts_after_speech_and_silence(self):
        websocket = FakeWebSocket()
        session = VoiceSession(websocket, "board", "session")
        session.state = "LISTENING"
        loud = array("h", [1200] * (PCM_FRAME_BYTES // 2)).tobytes()
        quiet = bytes(PCM_FRAME_BYTES)
        session.start_pipeline = AsyncMock()

        await session.consume_audio(quiet)
        self.assertFalse(session.speech_started)
        for _ in range(3):
            await session.consume_audio(loud)
        for _ in range(9):
            await session.consume_audio(quiet)

        self.assertTrue(session.speech_started)
        session.start_pipeline.assert_awaited_once()

    async def test_abort_stops_tts_and_returns_to_wake_mode(self):
        websocket = FakeWebSocket()
        session = VoiceSession(websocket, "board", "session")
        session.state = "SPEAKING"

        await session.abort()

        self.assertEqual(session.state, "WAKE_WORD")
        self.assertEqual(websocket.text_messages[0]["type"], "tts")
        self.assertEqual(websocket.text_messages[0]["state"], "stop")
        self.assertEqual(websocket.text_messages[-2]["state"], "wake")


if __name__ == "__main__":
    unittest.main()
