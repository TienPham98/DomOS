"""Host-side regression tests for ESP32-S3 hardware and protocol contracts."""

import re
import unittest
from pathlib import Path


FIRMWARE = Path(__file__).resolve().parents[1]
MAIN = FIRMWARE / "main"


def read(relative_path: str) -> str:
    return (FIRMWARE / relative_path).read_text(encoding="utf-8")


class BoardContractTests(unittest.TestCase):
    def test_es3c28p_pin_map_is_unchanged(self):
        config = read("main/board/es3c28p/board_config.h")
        expected = {
            "TFT_MOSI": 11, "TFT_MISO": 13, "TFT_SCLK": 12, "TFT_CS": 10,
            "TFT_DC": 46, "TFT_BL": 45, "TOUCH_SDA": 16, "TOUCH_SCL": 15,
            "TOUCH_RST": 18, "TOUCH_INT": 17, "AUDIO_MCLK": 4,
            "AUDIO_BCLK": 5, "AUDIO_DIN": 6, "AUDIO_WS": 7,
            "AUDIO_DOUT": 8, "AUDIO_PA": 1,
        }
        for name, gpio in expected.items():
            with self.subTest(name=name):
                self.assertRegex(config, rf"#define\s+{name}\s+GPIO_NUM_{gpio}\b")

    def test_codec_reset_and_audio_format_match_current_board(self):
        codec = read("main/board/es3c28p/es8311.cpp")
        self.assertIn("write_reg(ctrl_if_, 0x00, 1, &reset_value, 1)", codec)
        self.assertRegex(codec, r"reset_value\s*=\s*0x1F")
        self.assertIn("vTaskDelay(pdMS_TO_TICKS(5))", codec)
        self.assertIn("sample_info.bits_per_sample = 16", codec)
        self.assertIn("sample_info.channel = 1", codec)
        self.assertIn("std::clamp(gain_db, 0, 42)", codec)


class IntegrationContractTests(unittest.TestCase):
    def test_private_network_endpoints_are_injected(self):
        defaults = read("sdkconfig.defaults")
        wifi = read("main/services/wifi/wifi_service.cpp")
        self.assertIn('CONFIG_DOMOS_MQTT_URI=""', defaults)
        self.assertIn('CONFIG_DOMOS_AI_WS_URI=""', defaults)
        self.assertIn("CONFIG_DOMOS_DEVICE_IP", wifi)
        self.assertIn('std::strncpy(s_ssid, "Dom_12"', wifi)

    def test_audio_tasks_keep_realtime_core_and_priority_contract(self):
        pipeline = read("main/services/assistant/audio_pipeline.cpp")
        self.assertRegex(
            pipeline,
            r'xTaskCreatePinnedToCore\(MicTask,\s*"mic_capture",\s*4096,\s*this,\s*7,\s*&mic_handle,\s*1\)',
        )
        self.assertRegex(
            pipeline,
            r'xTaskCreatePinnedToCore\(OutputTask,\s*"audio_out",\s*4096,\s*this,\s*7,\s*&output_handle,\s*0\)',
        )

    def test_speaker_amplifier_is_only_controlled_by_assistant(self):
        callers = []
        for source in MAIN.rglob("*.cpp"):
            if source.as_posix().endswith("board/es3c28p/audio.cpp"):
                continue
            if "SetPAEnabled(" in source.read_text(encoding="utf-8"):
                callers.append(source.relative_to(MAIN).as_posix())
        self.assertEqual(callers, ["services/assistant/assistant_service.cpp"])

    def test_state_changes_publish_through_set_state(self):
        service = read("main/services/assistant/assistant_service.cpp")
        self.assertIn("state_.exchange(static_cast<uint8_t>(s))", service)
        self.assertIn("events_->Publish(EventType::AssistantState", service)
        outside_setter = service[:service.index("void AssistantService::SetState")]
        self.assertNotRegex(outside_setter, r"state_\s*(?:=|\.store\s*\()")

    def test_device_http_contract_is_registered(self):
        server = read("main/services/filesystem/upload_server.cpp")
        for route in ("/upload", "/api/status", "/api/logs", "/api/wallpaper"):
            with self.subTest(route=route):
                self.assertIn(f'.uri = "{route}"', server)


if __name__ == "__main__":
    unittest.main()
