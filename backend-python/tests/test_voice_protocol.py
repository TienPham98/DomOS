import tempfile
import unittest
from array import array
from pathlib import Path

from services.conversation_store import ConversationStore
from services.openrouter_voice_service import PCM_FRAME_BYTES, VAD_ENERGY_THRESHOLD, VAD_SILENCE_FRAMES, matches_device_wake_signature, normalize_wake_pcm, pcm_rms, pcm_to_wav, split_wake_word, validate_dom_hello


class VoiceProtocolTests(unittest.TestCase):
    def test_dom_protocol_remains_pcm_v3(self):
        validate_dom_hello({"type": "hello", "version": 3, "audio_params": {"codec": "pcm", "sample_rate": 16000, "channels": 1, "frame_duration": 60}})

    def test_vad_constants_match_sixty_millisecond_frames(self):
        self.assertEqual(PCM_FRAME_BYTES, 1920)
        self.assertEqual(VAD_ENERGY_THRESHOLD, 180)
        self.assertEqual(VAD_SILENCE_FRAMES, 9)
        self.assertEqual(pcm_rms(bytes(PCM_FRAME_BYTES)), 0)

    def test_pcm_is_wrapped_as_standard_wav(self):
        wav = pcm_to_wav(bytes(PCM_FRAME_BYTES))
        self.assertEqual(wav[:4], b"RIFF")
        self.assertEqual(wav[8:12], b"WAVE")

    def test_quiet_wake_audio_is_normalized_and_padded(self):
        quiet = array("h", [400] * (PCM_FRAME_BYTES // 2)).tobytes()
        normalized = normalize_wake_pcm(quiet)
        self.assertGreater(pcm_rms(normalized), pcm_rms(quiet))
        self.assertEqual(len(normalized), len(quiet) + 16000)

    def test_wake_word_accepts_supported_variants_and_extracts_command(self):
        self.assertEqual(split_wake_word("Hey Dom, tăng âm lượng"), (True, "tăng âm lượng"))
        self.assertEqual(split_wake_word("Hây Dom"), (True, ""))
        self.assertEqual(split_wake_word("Hello kể chuyện cười"), (True, "kể chuyện cười"))
        self.assertEqual(split_wake_word("hello Tom"), (True, ""))
        self.assertEqual(split_wake_word("Hey dog"), (True, ""))
        self.assertEqual(split_wake_word("he to"), (True, ""))
        self.assertEqual(split_wake_word("he do"), (True, ""))
        self.assertEqual(split_wake_word("He does"), (True, ""))
        self.assertEqual(split_wake_word("hey don't hey"), (False, ""))
        self.assertEqual(split_wake_word("Hey Don mở đồng hồ"), (True, "mở đồng hồ"))
        self.assertEqual(split_wake_word("huy động"), (True, ""))
        self.assertEqual(split_wake_word("Dom tăng độ sáng"), (True, "tăng độ sáng"))
        self.assertEqual(split_wake_word("xin chào Dom"), (False, ""))

    def test_bilingual_device_signature_requires_both_recognizers(self):
        self.assertTrue(matches_device_wake_signature("I don't", "hành động"))
        self.assertTrue(matches_device_wake_signature("are you done", "thầy chọn"))
        self.assertTrue(matches_device_wake_signature("I don't", "thấy chưa"))
        self.assertTrue(matches_device_wake_signature("I don't", "cây thông"))
        self.assertTrue(matches_device_wake_signature("hey don't", "hình tròn"))
        self.assertFalse(matches_device_wake_signature("I don't", "xin chào"))
        self.assertFalse(matches_device_wake_signature("good morning", "hình tròn"))


class ConversationStoreTests(unittest.IsolatedAsyncioTestCase):
    async def test_turn_and_tool_trace_are_persistent_context(self):
        with tempfile.TemporaryDirectory() as directory:
            store = ConversationStore(str(Path(directory) / "test.db"))
            await store.initialize()
            turn_id = await store.create_turn("board", "tăng âm lượng", "openrouter", "openrouter/free")
            await store.add_tool_trace(turn_id, "speaker.adjust_volume", {"delta": 10}, {"isError": False}, 56, "success")
            await store.complete_turn(turn_id, "Âm lượng đã tăng rồi nhé!")
            turns = await store.list_turns("board")
            context = await store.recent_context("board")
            self.assertEqual(turns[0]["tool_calls"][0]["duration_ms"], 56)
            self.assertEqual(context[-1]["role"], "assistant")
            self.assertIn("đã tăng", context[-1]["content"])


if __name__ == "__main__":
    unittest.main()
