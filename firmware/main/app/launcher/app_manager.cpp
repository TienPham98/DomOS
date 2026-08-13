#define LV_USE_SJPG 1
#include "app_manager.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>

#include "app/i_app.h"
#include "board/es3c28p/board_es3c28p.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "lvgl.h"
#include "extra/libs/sjpg/tjpgd.h"
#include "services/media/media_service.h"
#include "services/mqtt/mqtt_service.h"
#include "services/ota/ota_service.h"
#include "services/wifi/wifi_service.h"
#include "services/filesystem/upload_server.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <vector>


namespace {


lv_obj_t *Label(lv_obj_t *parent, const char *text, lv_align_t alignment, int x, int y, const lv_font_t *font = nullptr)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    if (font != nullptr) lv_obj_set_style_text_font(label, font, 0);
    lv_obj_align(label, alignment, x, y);
    return label;
}

lv_obj_t *Button(lv_obj_t *parent, const char *text, lv_align_t alignment, int x, int y, lv_event_cb_t callback,
                 void *context)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 138, 48);
    lv_obj_align(button, alignment, x, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_grad_color(button, lv_color_hex(0x1976D2), 0);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(button, 12, 0);
    lv_obj_set_style_shadow_width(button, 8, 0);
    lv_obj_set_style_shadow_opa(button, LV_OPA_30, 0);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, context);
    Label(button, text, LV_ALIGN_CENTER, 0, 0);
    return button;
}

void ToWallpaper(lv_event_t *event) { static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("wallpaper"); }
void ToAssistant(lv_event_t *event) { static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("assistant"); }
void ToSmartHome(lv_event_t *event) { static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("smart-home"); }
void ToSettings(lv_event_t *event) { static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("settings"); }
void ToOta(lv_event_t *event) { static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("ota"); }

class ScreenApp : public IApp {
public:
    explicit ScreenApp(AppManager &manager) : manager_(manager) {}
    void Destroy() override
    {
        if (screen_ != nullptr) lv_obj_del(screen_);
        screen_ = nullptr;
    }
    void Show() override { lv_scr_load(screen_); }
    void Hide() override {}

protected:
    void AddBackButton()
    {
        lv_obj_t *top_btn = lv_btn_create(screen_);
        lv_obj_set_size(top_btn, 48, 24);
        lv_obj_align(top_btn, LV_ALIGN_BOTTOM_LEFT, 6, -6);
        lv_obj_set_style_bg_color(top_btn, lv_color_hex(0x37474F), 0);
        lv_obj_set_style_bg_grad_color(top_btn, lv_color_hex(0x263238), 0);
        lv_obj_set_style_bg_grad_dir(top_btn, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_radius(top_btn, 6, 0);
        lv_obj_set_style_shadow_width(top_btn, 0, 0);
        lv_obj_set_style_pad_all(top_btn, 0, 0);
        lv_obj_add_event_cb(top_btn, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->CloseCurrent();
        }, LV_EVENT_CLICKED, &manager_);
        lv_obj_t *lbl = lv_label_create(top_btn);
        lv_label_set_text(lbl, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    AppManager &manager_;
    lv_obj_t *screen_ = nullptr;
};

class LauncherApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "launcher"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen_, lv_color_hex(0x121212), 0);
        Label(screen_, "DomOS", LV_ALIGN_TOP_LEFT, 16, 12);
        Label(screen_, "Home", LV_ALIGN_TOP_RIGHT, -16, 14);
        Button(screen_, "Wallpaper", LV_ALIGN_CENTER, -75, -28, ToWallpaper, &manager_);
        Button(screen_, "AI", LV_ALIGN_CENTER, 75, -28, ToAssistant, &manager_);
        Button(screen_, "Smart Home", LV_ALIGN_CENTER, -75, 32, ToSmartHome, &manager_);
        Button(screen_, "Settings", LV_ALIGN_CENTER, 75, 32, ToSettings, &manager_);
        lv_obj_add_event_cb(screen_, [](lv_event_t *event) {
            lv_indev_t *indev = lv_indev_get_act();
            if (indev != nullptr && lv_indev_get_gesture_dir(indev) == LV_DIR_LEFT) {
                static_cast<AppManager *>(lv_event_get_user_data(event))->ShowNextHomePage();
            }
        }, LV_EVENT_GESTURE, &manager_);
        AddBackButton();
    }
};

namespace {
const char* NumberToWord(int n) {
    static const char* words[] = {
        "ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE",
        "TEN", "ELEVEN", "TWELVE", "THIRTEEN", "FOURTEEN", "FIFTEEN", "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN",
        "TWENTY", "TWENTY-ONE", "TWENTY-TWO", "TWENTY-THREE", "TWENTY-FOUR", "TWENTY-FIVE", "TWENTY-SIX", "TWENTY-SEVEN", "TWENTY-EIGHT", "TWENTY-NINE",
        "THIRTY", "THIRTY-ONE", "THIRTY-TWO", "THIRTY-THREE", "THIRTY-FOUR", "THIRTY-FIVE", "THIRTY-SIX", "THIRTY-SEVEN", "THIRTY-EIGHT", "THIRTY-NINE",
        "FORTY", "FORTY-ONE", "FORTY-TWO", "FORTY-THREE", "FORTY-FOUR", "FORTY-FIVE", "FORTY-SIX", "FORTY-SEVEN", "FORTY-EIGHT", "FORTY-NINE",
        "FIFTY", "FIFTY-ONE", "FIFTY-TWO", "FIFTY-THREE", "FIFTY-FOUR", "FIFTY-FIVE", "FIFTY-SIX", "FIFTY-SEVEN", "FIFTY-EIGHT", "FIFTY-NINE"
    };
    if (n >= 0 && n < 60) return words[n];
    return "";
}
} // namespace

class ClockApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "clock"; }
    void Create() override
    {
        LoadNvs();
        const bool is_light = (mode_ == "light");
        const lv_color_t bg_col = is_light ? lv_color_hex(0xF1F5F9) : lv_color_hex(0x000000);
        const lv_color_t sub_col = is_light ? lv_color_hex(0x475569) : lv_color_hex(0x94a3b8);

        screen_ = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen_, bg_col, 0);
        lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(screen_, LV_SCROLLBAR_MODE_OFF);
        Label(screen_, "DomOS", LV_ALIGN_TOP_RIGHT, -12, 10);

        // 1. Digital / Minimal Text Labels
        time_ = Label(screen_, "--:--", LV_ALIGN_CENTER, 0, -32);
        lv_obj_set_style_text_font(time_, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(time_, color_, 0);

        separator_ = lv_obj_create(screen_);
        lv_obj_set_size(separator_, 48, 2);
        lv_obj_align(separator_, LV_ALIGN_CENTER, 0, 8);
        lv_obj_set_style_bg_color(separator_, color_, 0);
        lv_obj_set_style_border_width(separator_, 0, 0);

        date_ = Label(screen_, "Waiting for Wi-Fi time", LV_ALIGN_CENTER, 0, 28);
        lv_obj_set_style_text_color(date_, sub_col, 0);

        // 2. Futuristic Arc Gauge (Replaces Analog)
        arc_container_ = lv_obj_create(screen_);
        lv_obj_set_size(arc_container_, 165, 165);
        lv_obj_align(arc_container_, LV_ALIGN_CENTER, 0, -18);
        lv_obj_set_style_bg_opa(arc_container_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(arc_container_, 0, 0);
        lv_obj_set_style_pad_all(arc_container_, 0, 0);
        lv_obj_clear_flag(arc_container_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(arc_container_, LV_SCROLLBAR_MODE_OFF);

        arc_sec_ = lv_arc_create(arc_container_);
        lv_obj_set_size(arc_sec_, 156, 156);
        lv_obj_align(arc_sec_, LV_ALIGN_CENTER, 0, 0);
        lv_arc_set_rotation(arc_sec_, 270);
        lv_arc_set_bg_angles(arc_sec_, 0, 360);
        lv_arc_set_range(arc_sec_, 0, 60);
        lv_obj_remove_style(arc_sec_, nullptr, LV_PART_KNOB);
        lv_obj_clear_flag(arc_sec_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(arc_sec_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_arc_width(arc_sec_, 6, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc_sec_, 6, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc_sec_, lv_color_hex(0x0f172a), LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc_sec_, color_, LV_PART_INDICATOR);

        arc_min_ = lv_arc_create(arc_container_);
        lv_obj_set_size(arc_min_, 138, 138);
        lv_obj_align(arc_min_, LV_ALIGN_CENTER, 0, 0);
        lv_arc_set_rotation(arc_min_, 270);
        lv_arc_set_bg_angles(arc_min_, 0, 360);
        lv_arc_set_range(arc_min_, 0, 60);
        lv_obj_remove_style(arc_min_, nullptr, LV_PART_KNOB);
        lv_obj_clear_flag(arc_min_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(arc_min_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_arc_width(arc_min_, 4, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc_min_, 4, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc_min_, lv_color_hex(0x1e293b), LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc_min_, lv_color_hex(0x475569), LV_PART_INDICATOR);

        arc_time_ = Label(arc_container_, "--:--", LV_ALIGN_CENTER, 0, -8);
        lv_obj_set_style_text_font(arc_time_, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(arc_time_, color_, 0);

        arc_sec_lbl_ = Label(arc_container_, "-- SEC", LV_ALIGN_CENTER, 0, 24);
        lv_obj_set_style_text_color(arc_sec_lbl_, lv_color_hex(0x94a3b8), 0);

        // 3. Flip Clock Container
        flip_container_ = lv_obj_create(screen_);
        lv_obj_set_size(flip_container_, 230, 64);
        lv_obj_align(flip_container_, LV_ALIGN_CENTER, 0, -18);
        lv_obj_set_style_bg_opa(flip_container_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(flip_container_, 0, 0);
        lv_obj_set_style_pad_all(flip_container_, 0, 0);
        lv_obj_clear_flag(flip_container_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(flip_container_, LV_SCROLLBAR_MODE_OFF);

        const int flip_offsets[4] = {-88, -44, 44, 88};
        for (int i = 0; i < 4; ++i) {
            flip_cards_[i] = lv_obj_create(flip_container_);
            lv_obj_set_size(flip_cards_[i], 38, 52);
            lv_obj_align(flip_cards_[i], LV_ALIGN_CENTER, flip_offsets[i], 0);
            lv_obj_set_style_bg_color(flip_cards_[i], lv_color_hex(0x0f172a), 0);
            lv_obj_set_style_border_color(flip_cards_[i], lv_color_hex(0x334155), 0);
            lv_obj_set_style_border_width(flip_cards_[i], 1, 0);
            lv_obj_set_style_radius(flip_cards_[i], 8, 0);
            lv_obj_clear_flag(flip_cards_[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(flip_cards_[i], LV_SCROLLBAR_MODE_OFF);

            flip_labels_[i] = Label(flip_cards_[i], "0", LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(flip_labels_[i], color_, 0);
        }
        flip_colon_ = Label(flip_container_, ":", LV_ALIGN_CENTER, 0, -2);
        lv_obj_set_style_text_color(flip_colon_, lv_color_hex(0x64748b), 0);

        // 4. Word Clock Container
        word_container_ = lv_obj_create(screen_);
        lv_obj_set_size(word_container_, 250, 110);
        lv_obj_align(word_container_, LV_ALIGN_CENTER, 0, -18);
        lv_obj_set_style_bg_opa(word_container_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(word_container_, 0, 0);
        lv_obj_set_style_pad_all(word_container_, 0, 0);
        lv_obj_clear_flag(word_container_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(word_container_, LV_SCROLLBAR_MODE_OFF);

        word_l1_ = Label(word_container_, "IT IS", LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_text_color(word_l1_, lv_color_hex(0x64748b), 0);

        word_l2_ = Label(word_container_, "--", LV_ALIGN_TOP_MID, 0, 20);
        lv_obj_set_style_text_color(word_l2_, color_, 0);

        word_l3_ = Label(word_container_, "MINUTES PAST", LV_ALIGN_TOP_MID, 0, 42);
        lv_obj_set_style_text_color(word_l3_, lv_color_hex(0x64748b), 0);

        word_l4_ = Label(word_container_, "--", LV_ALIGN_TOP_MID, 0, 62);
        lv_obj_set_style_text_color(word_l4_, color_, 0);

        // 5. Binary Clock Container
        binary_container_ = lv_obj_create(screen_);
        lv_obj_set_size(binary_container_, 150, 110);
        lv_obj_align(binary_container_, LV_ALIGN_CENTER, 0, -18);
        lv_obj_set_style_bg_opa(binary_container_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(binary_container_, 0, 0);
        lv_obj_set_style_pad_all(binary_container_, 0, 0);
        lv_obj_clear_flag(binary_container_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(binary_container_, LV_SCROLLBAR_MODE_OFF);

        for (int i = 0; i < 5; ++i) {
            hour_bits_[i] = lv_obj_create(binary_container_);
            lv_obj_set_size(hour_bits_[i], 14, 14);
            lv_obj_align(hour_bits_[i], LV_ALIGN_TOP_LEFT, 30, i * 17);
            lv_obj_set_style_radius(hour_bits_[i], 3, 0);
            lv_obj_set_style_bg_color(hour_bits_[i], lv_color_hex(0x1e293b), 0);
            lv_obj_set_style_border_width(hour_bits_[i], 0, 0);
            lv_obj_clear_flag(hour_bits_[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(hour_bits_[i], LV_SCROLLBAR_MODE_OFF);
        }
        for (int i = 0; i < 6; ++i) {
            min_bits_[i] = lv_obj_create(binary_container_);
            lv_obj_set_size(min_bits_[i], 14, 14);
            lv_obj_align(min_bits_[i], LV_ALIGN_TOP_RIGHT, -30, i * 15);
            lv_obj_set_style_radius(min_bits_[i], 3, 0);
            lv_obj_set_style_bg_color(min_bits_[i], lv_color_hex(0x1e293b), 0);
            lv_obj_set_style_border_width(min_bits_[i], 0, 0);
            lv_obj_clear_flag(min_bits_[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_scrollbar_mode(min_bits_[i], LV_SCROLLBAR_MODE_OFF);
        }

        // Menu button at bottom left to launch App List (Launcher)
        menu_btn_ = lv_btn_create(screen_);
        lv_obj_set_size(menu_btn_, 48, 24);
        lv_obj_align(menu_btn_, LV_ALIGN_BOTTOM_LEFT, 6, -6);
        lv_obj_set_style_bg_color(menu_btn_, is_light ? lv_color_hex(0xe2e8f0) : lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_radius(menu_btn_, 6, 0);
        lv_obj_set_style_pad_all(menu_btn_, 0, 0);
        lv_obj_add_event_cb(menu_btn_, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("launcher");
        }, LV_EVENT_CLICKED, &manager_);
        lv_obj_t *menu_lbl = lv_label_create(menu_btn_);
        lv_label_set_text(menu_lbl, LV_SYMBOL_LIST);
        lv_obj_set_style_text_color(menu_lbl, is_light ? lv_color_hex(0x0f172a) : lv_color_hex(0xf8fafc), 0);
        lv_obj_set_style_text_font(menu_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(menu_lbl, LV_ALIGN_CENTER, 0, 0);

        timer_ = lv_timer_create([](lv_timer_t *timer) { static_cast<ClockApp *>(timer->user_data)->Refresh(); }, 1000, this);
        ApplyLayout();
    }

    std::string GetStyle() const { return style_name_; }
    uint32_t GetColorHex() const { return color_hex_; }
    std::string GetMode() const { return mode_; }

    void LoadNvs()
    {
        nvs_handle_t nvs;
        if (nvs_open("clock_cfg", NVS_READONLY, &nvs) == ESP_OK) {
            char buf[32] = {};
            size_t len = sizeof(buf);
            if (nvs_get_str(nvs, "style", buf, &len) == ESP_OK && buf[0] != '\0') {
                style_name_ = buf;
            }
            uint32_t val = 0;
            if (nvs_get_u32(nvs, "color", &val) == ESP_OK) {
                color_hex_ = val;
                color_ = lv_color_hex(color_hex_);
            }
            char mode_buf[16] = {};
            len = sizeof(mode_buf);
            if (nvs_get_str(nvs, "mode", mode_buf, &len) == ESP_OK && mode_buf[0] != '\0') {
                mode_ = mode_buf;
            }
            nvs_close(nvs);
        }
    }

    void SaveNvs()
    {
        nvs_handle_t nvs;
        if (nvs_open("clock_cfg", NVS_READWRITE, &nvs) == ESP_OK) {
            nvs_set_str(nvs, "style", style_name_.c_str());
            nvs_set_u32(nvs, "color", color_hex_);
            nvs_set_str(nvs, "mode", mode_.c_str());
            nvs_commit(nvs);
            nvs_close(nvs);
        }
    }

    void SetStyle(const std::string &style, uint32_t color_hex, const std::string &mode = "")
    {
        if (!style.empty()) style_name_ = style;
        if (color_hex != 0) {
            color_hex_ = color_hex;
            color_ = lv_color_hex(color_hex_);
        }
        if (!mode.empty()) mode_ = mode;

        const bool is_light = (mode_ == "light");
        const lv_color_t bg_col = is_light ? lv_color_hex(0xF1F5F9) : lv_color_hex(0x000000);
        const lv_color_t sub_col = is_light ? lv_color_hex(0x475569) : lv_color_hex(0x94a3b8);

        if (screen_ != nullptr) lv_obj_set_style_bg_color(screen_, bg_col, 0);

        if (time_ != nullptr) lv_obj_set_style_text_color(time_, color_, 0);
        if (separator_ != nullptr) lv_obj_set_style_bg_color(separator_, color_, 0);
        if (date_ != nullptr) lv_obj_set_style_text_color(date_, sub_col, 0);

        if (arc_sec_ != nullptr) lv_obj_set_style_arc_color(arc_sec_, color_, LV_PART_INDICATOR);
        if (arc_time_ != nullptr) lv_obj_set_style_text_color(arc_time_, color_, 0);
        if (arc_sec_lbl_ != nullptr) lv_obj_set_style_text_color(arc_sec_lbl_, sub_col, 0);

        if (word_l2_ != nullptr) lv_obj_set_style_text_color(word_l2_, color_, 0);
        if (word_l4_ != nullptr) lv_obj_set_style_text_color(word_l4_, color_, 0);

        for (int i = 0; i < 4; ++i) {
            if (flip_cards_[i] != nullptr) lv_obj_set_style_bg_color(flip_cards_[i], is_light ? lv_color_hex(0xffffff) : lv_color_hex(0x0f172a), 0);
            if (flip_labels_[i] != nullptr) lv_obj_set_style_text_color(flip_labels_[i], color_, 0);
        }
        if (menu_btn_ != nullptr) {
            lv_obj_set_style_bg_color(menu_btn_, is_light ? lv_color_hex(0xe2e8f0) : lv_color_hex(0x1e293b), 0);
        }

        ApplyLayout();
        Refresh();
        SaveNvs();
    }

    void Destroy() override
    {
        if (timer_ != nullptr) lv_timer_del(timer_);
        timer_ = nullptr;
        ScreenApp::Destroy();
    }

    void Show() override
    {
        ScreenApp::Show();
        Refresh();
    }

private:
    void ApplyLayout()
    {
        if (screen_ == nullptr) return;
        lv_obj_add_flag(arc_container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(flip_container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(word_container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(binary_container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(time_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(separator_, LV_OBJ_FLAG_HIDDEN);

        if (style_name_ == "analog" || style_name_ == "arc" || style_name_ == "gauge") {
            lv_obj_clear_flag(arc_container_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(date_, LV_ALIGN_CENTER, 0, 68);
        } else if (style_name_ == "flip") {
            lv_obj_clear_flag(flip_container_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(date_, LV_ALIGN_CENTER, 0, 32);
        } else if (style_name_ == "word") {
            lv_obj_clear_flag(word_container_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(date_, LV_ALIGN_CENTER, 0, 48);
        } else if (style_name_ == "binary") {
            lv_obj_clear_flag(binary_container_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(date_, LV_ALIGN_CENTER, 0, 45);
        } else if (style_name_ == "minimal") {
            lv_obj_clear_flag(time_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(separator_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(time_, LV_ALIGN_CENTER, 0, -30);
            lv_obj_align(date_, LV_ALIGN_CENTER, 0, 25);
        } else { // digital / default
            lv_obj_clear_flag(time_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(time_, LV_ALIGN_CENTER, 0, -25);
            lv_obj_align(date_, LV_ALIGN_CENTER, 0, 15);
        }
    }

    void Refresh()
    {
        std::time_t now = std::time(nullptr);
        std::tm time_info{};
        localtime_r(&now, &time_info);

        char time_text[12] = "--:--";
        char date_text[32] = "Waiting for Wi-Fi time";

        if (time_info.tm_year >= 120) {
            std::strftime(time_text, sizeof(time_text), "%H:%M", &time_info);
            if (style_name_ == "minimal") {
                static const char *const days[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};
                std::strncpy(date_text, days[time_info.tm_wday % 7], sizeof(date_text) - 1);
            } else if (style_name_ == "digital") {
                char day_buf[8] = {}, month_buf[8] = {};
                std::strftime(day_buf, sizeof(day_buf), "%a", &time_info);
                std::strftime(month_buf, sizeof(month_buf), "%b", &time_info);
                for (auto &c : day_buf) c = std::toupper(c);
                for (auto &c : month_buf) c = std::toupper(c);
                std::snprintf(date_text, sizeof(date_text), "%s . %s %d", day_buf, month_buf, time_info.tm_mday);
            } else {
                std::strftime(date_text, sizeof(date_text), "%a, %b %d", &time_info);
            }

            if (style_name_ == "flip" && flip_labels_[0] != nullptr) {
                char h1[2] = {static_cast<char>('0' + (time_info.tm_hour / 10)), '\0'};
                char h2[2] = {static_cast<char>('0' + (time_info.tm_hour % 10)), '\0'};
                char m1[2] = {static_cast<char>('0' + (time_info.tm_min / 10)), '\0'};
                char m2[2] = {static_cast<char>('0' + (time_info.tm_min % 10)), '\0'};
                lv_label_set_text(flip_labels_[0], h1);
                lv_label_set_text(flip_labels_[1], h2);
                lv_label_set_text(flip_labels_[2], m1);
                lv_label_set_text(flip_labels_[3], m2);
            } else if (style_name_ == "word" && word_l2_ != nullptr) {
                lv_label_set_text(word_l2_, NumberToWord(time_info.tm_min));
                lv_label_set_text(word_l4_, NumberToWord(time_info.tm_hour % 12 == 0 ? 12 : time_info.tm_hour % 12));
            } else if (style_name_ == "binary" && hour_bits_[0] != nullptr) {
                const int h = time_info.tm_hour;
                const int m = time_info.tm_min;
                for (int i = 0; i < 5; ++i) {
                    const bool bit_on = (h & (1 << (4 - i))) != 0;
                    lv_obj_set_style_bg_color(hour_bits_[i], bit_on ? color_ : lv_color_hex(0x1e293b), 0);
                }
                for (int i = 0; i < 6; ++i) {
                    const bool bit_on = (m & (1 << (5 - i))) != 0;
                    lv_obj_set_style_bg_color(min_bits_[i], bit_on ? color_ : lv_color_hex(0x1e293b), 0);
                }
            }
        }
        if (time_ != nullptr) lv_label_set_text(time_, time_text);
        if (date_ != nullptr) lv_label_set_text(date_, date_text);

        // Update Arc Gauge values if arc/gauge/analog mode
        if ((style_name_ == "analog" || style_name_ == "arc" || style_name_ == "gauge") && arc_sec_ != nullptr) {
            lv_arc_set_value(arc_sec_, time_info.tm_sec);
            lv_arc_set_value(arc_min_, time_info.tm_min);
            lv_label_set_text(arc_time_, time_text);
            char sec_buf[16];
            std::snprintf(sec_buf, sizeof(sec_buf), "%02d SEC", time_info.tm_sec);
            lv_label_set_text(arc_sec_lbl_, sec_buf);
        }
    }

    lv_obj_t *time_ = nullptr;
    lv_obj_t *separator_ = nullptr;
    lv_obj_t *date_ = nullptr;

    lv_obj_t *arc_container_ = nullptr;
    lv_obj_t *arc_sec_ = nullptr;
    lv_obj_t *arc_min_ = nullptr;
    lv_obj_t *arc_time_ = nullptr;
    lv_obj_t *arc_sec_lbl_ = nullptr;

    lv_obj_t *flip_container_ = nullptr;
    lv_obj_t *flip_cards_[4]{};
    lv_obj_t *flip_labels_[4]{};
    lv_obj_t *flip_colon_ = nullptr;

    lv_obj_t *word_container_ = nullptr;
    lv_obj_t *word_l1_ = nullptr;
    lv_obj_t *word_l2_ = nullptr;
    lv_obj_t *word_l3_ = nullptr;
    lv_obj_t *word_l4_ = nullptr;

    lv_obj_t *binary_container_ = nullptr;
    lv_obj_t *hour_bits_[5]{};
    lv_obj_t *min_bits_[6]{};

    lv_timer_t *timer_ = nullptr;
    std::string style_name_ = "digital";
    uint32_t color_hex_ = 0x06b6d4;
    std::string mode_ = "light";
    lv_color_t color_ = lv_color_hex(0x06b6d4);
    lv_obj_t *menu_btn_ = nullptr;
};

static std::string s_active_wallpaper_url;
static std::string s_active_wallpaper_name;
static bool s_slideshow_active = false;
static int s_slideshow_interval_sec = 30;
static int s_current_slot_idx = 0;
static const char *s_slot_filepaths[3] = {
    "/littlefs/wallpapers/current.jpg",
    "/littlefs/wallpapers/next.jpg",
    "/littlefs/wallpapers/next2.jpg"
};
static std::vector<std::string> s_playlist_urls;
static size_t s_playlist_idx = 0;

namespace {
struct JpegDecodeCtx {
    FILE *fp;
    ES3C28PBoard *board;
};

static size_t tjpgd_file_in_func(JDEC *jd, uint8_t *buff, size_t nbyte)
{
    auto *ctx = static_cast<JpegDecodeCtx *>(jd->device);
    if (ctx == nullptr || ctx->fp == nullptr) return 0;
    if (buff == nullptr) {
        std::fseek(ctx->fp, nbyte, SEEK_CUR);
        return nbyte;
    }
    return std::fread(buff, 1, nbyte, ctx->fp);
}

static int tjpgd_stream_out_func(JDEC *jd, void *bitmap, JRECT *rect)
{
    auto *ctx = static_cast<JpegDecodeCtx *>(jd->device);
    if (ctx == nullptr || ctx->board == nullptr || ctx->board->Panel() == nullptr) return 0;

    const uint8_t *src_rgb888 = static_cast<const uint8_t *>(bitmap);
    const uint32_t box_w = rect->right - rect->left + 1;
    const uint32_t box_h = rect->bottom - rect->top + 1;

    static uint16_t *dma_buf_0 = nullptr;
    static uint16_t *dma_buf_1 = nullptr;
    static bool toggle_buf = false;

    if (dma_buf_0 == nullptr) {
        dma_buf_0 = static_cast<uint16_t *>(heap_caps_malloc(16 * 16 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        dma_buf_1 = static_cast<uint16_t *>(heap_caps_malloc(16 * 16 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    }
    uint16_t *dma_block_buf = toggle_buf ? dma_buf_1 : dma_buf_0;
    toggle_buf = !toggle_buf;

    if (dma_block_buf == nullptr || box_w * box_h > 16 * 16) return 0;

    for (uint32_t y = 0; y < box_h; ++y) {
        const uint8_t *src_row = src_rgb888 + (y * box_w * 3);
        uint16_t *dst_row = dma_block_buf + (y * box_w);
        for (uint32_t x = 0; x < box_w; ++x) {
            uint8_t r = src_row[x * 3 + 0];
            uint8_t g = src_row[x * 3 + 1];
            uint8_t b = src_row[x * 3 + 2];
            uint16_t raw_rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            dst_row[x] = (raw_rgb565 >> 8) | (raw_rgb565 << 8);
        }
    }

    esp_err_t err = esp_lcd_panel_draw_bitmap(ctx->board->Panel(), rect->left, rect->top, rect->right + 1, rect->bottom + 1, dma_block_buf);
    if (err != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(1));
        esp_lcd_panel_draw_bitmap(ctx->board->Panel(), rect->left, rect->top, rect->right + 1, rect->bottom + 1, dma_block_buf);
    }
    return 1;
}



static bool StreamDecodeJpegFileToPanel(const char *filepath, ES3C28PBoard *board)
{
    if (filepath == nullptr || board == nullptr) return false;
    FILE *fp = std::fopen(filepath, "rb");
    if (fp == nullptr) {
        ESP_LOGE("wallpaper", "Failed to open image file: %s", filepath);
        AddSystemLog("WARN", "wallpaper", "File not found: %s", filepath);
        return false;
    }

    uint8_t *work_buf = static_cast<uint8_t *>(heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (work_buf == nullptr) {
        std::fclose(fp);
        return false;
    }

    JpegDecodeCtx decode_ctx{fp, board};
    JDEC jdec{};
    jdec.device = &decode_ctx;

    JRESULT res = jd_prepare(&jdec, tjpgd_file_in_func, work_buf, 4096, &decode_ctx);
    if (res == JDR_OK) {
        res = jd_decomp(&jdec, tjpgd_stream_out_func, 0);
        if (res == JDR_OK) {
            ESP_LOGI("wallpaper", "JPEG stream decode successful for %s", filepath);
            AddSystemLog("INFO", "wallpaper", "Decoded JPEG %s to panel (320x240)", filepath);
        } else {
            ESP_LOGE("wallpaper", "jd_decomp failed: %d", res);
            AddSystemLog("ERROR", "wallpaper", "jd_decomp err: %d for %s", res, filepath);
        }
    } else {
        ESP_LOGE("wallpaper", "jd_prepare failed: %d", res);
        AddSystemLog("ERROR", "wallpaper", "jd_prepare err: %d for %s", res, filepath);
    }
    heap_caps_free(work_buf);
    std::fclose(fp);
    return res == JDR_OK;
}


static bool DownloadUrlToFile(const std::string &url, const char *dest_path)
{
    if (url.empty() || dest_path == nullptr) return false;
    esp_http_client_config_t http_cfg{};
    http_cfg.url = url.c_str();
    http_cfg.timeout_ms = 8000;
    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    if (client == nullptr) return false;

    bool ok = false;
    if (esp_http_client_open(client, 0) == ESP_OK) {
        int content_len = esp_http_client_fetch_headers(client);
        if (content_len < 2 * 1024 * 1024) {
            FILE *fp = std::fopen(dest_path, "wb");
            if (fp != nullptr) {
                char buf[1024];
                int read_bytes = 0;
                int total = 0;
                while ((read_bytes = esp_http_client_read(client, buf, sizeof(buf))) > 0) {
                    std::fwrite(buf, 1, read_bytes, fp);
                    total += read_bytes;
                }
                std::fclose(fp);
                ok = (total > 0);
                if (ok) {
                    AddSystemLog("INFO", "wallpaper", "Downloaded %d bytes from %s to %s", total, url.c_str(), dest_path);
                } else {
                    AddSystemLog("WARN", "wallpaper", "Downloaded 0 bytes from %s", url.c_str());
                }
            } else {
                AddSystemLog("ERROR", "wallpaper", "Failed to open dest_path for write: %s", dest_path);
            }
        }
    } else {
        AddSystemLog("ERROR", "wallpaper", "Failed HTTP GET: %s", url.c_str());
    }
    esp_http_client_cleanup(client);
    return ok;
}


} // namespace

class WallpaperApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "wallpaper"; }

    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen_, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);
        lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(screen_, LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(screen_, LV_OBJ_FLAG_CLICKABLE);




        // Back Button (Top Left)
        back_btn_ = lv_btn_create(screen_);
        lv_obj_set_size(back_btn_, 40, 26);
        lv_obj_align(back_btn_, LV_ALIGN_TOP_LEFT, 6, 6);
        lv_obj_set_style_bg_opa(back_btn_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_shadow_width(back_btn_, 0, 0);
        lv_obj_set_style_border_width(back_btn_, 0, 0);
        lv_obj_set_style_pad_all(back_btn_, 0, 0);
        lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN); // Hidden by default
        lv_obj_add_event_cb(back_btn_, [](lv_event_t *event) {
            auto *wp_app = static_cast<WallpaperApp *>(lv_event_get_user_data(event));
            if (wp_app != nullptr) {
                uint32_t now = esp_log_timestamp();
                if (wp_app->last_back_click_ms_ != 0 && now - wp_app->last_back_click_ms_ < 600) {
                    // Double-click within 600ms: close app and return to home screen
                    wp_app->last_back_click_ms_ = 0;
                    if (wp_app->auto_hide_timer_) lv_timer_pause(wp_app->auto_hide_timer_);
                    wp_app->manager_.CloseCurrent();
                } else {
                    // First click: record timestamp, keep buttons visible, reset auto-hide timer
                    wp_app->last_back_click_ms_ = now;
                    wp_app->ResetAutoHideTimer();
                }
            }
        }, LV_EVENT_CLICKED, this);

        lv_obj_t *b_lbl = lv_label_create(back_btn_);
        lv_label_set_text(b_lbl, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(b_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(b_lbl, LV_ALIGN_CENTER, 0, 0);

        // Prev Arrow Button (<) (Compact 28x28)
        prev_btn_ = lv_btn_create(screen_);
        lv_obj_set_size(prev_btn_, 28, 28);
        lv_obj_align(prev_btn_, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_set_style_bg_opa(prev_btn_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_shadow_width(prev_btn_, 0, 0);
        lv_obj_set_style_border_width(prev_btn_, 0, 0);
        lv_obj_set_style_pad_all(prev_btn_, 0, 0);
        lv_obj_add_flag(prev_btn_, LV_OBJ_FLAG_HIDDEN); // Hidden by default
        lv_obj_add_event_cb(prev_btn_, [](lv_event_t *e) {
            auto *wp_app = static_cast<WallpaperApp *>(lv_event_get_user_data(e));
            if (wp_app != nullptr) {
                wp_app->manager_.TriggerPrevSlide();
                wp_app->RenderActiveSlot();
                lv_refr_now(nullptr);
                wp_app->ResetAutoHideTimer();
            }
        }, LV_EVENT_CLICKED, this);
        lv_obj_t *p_lbl = lv_label_create(prev_btn_);
        lv_label_set_text(p_lbl, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(p_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(p_lbl, LV_ALIGN_CENTER, 0, 0);

        // Next Arrow Button (>) (Compact 28x28)
        next_btn_ = lv_btn_create(screen_);
        lv_obj_set_size(next_btn_, 28, 28);
        lv_obj_align(next_btn_, LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_set_style_bg_opa(next_btn_, LV_OPA_TRANSP, 0);
        lv_obj_set_style_shadow_width(next_btn_, 0, 0);
        lv_obj_set_style_border_width(next_btn_, 0, 0);
        lv_obj_set_style_pad_all(next_btn_, 0, 0);
        lv_obj_add_flag(next_btn_, LV_OBJ_FLAG_HIDDEN); // Hidden by default
        lv_obj_add_event_cb(next_btn_, [](lv_event_t *e) {
            auto *wp_app = static_cast<WallpaperApp *>(lv_event_get_user_data(e));
            if (wp_app != nullptr) {
                wp_app->manager_.TriggerNextSlide();
                wp_app->RenderActiveSlot();
                lv_refr_now(nullptr);
                wp_app->ResetAutoHideTimer();
            }
        }, LV_EVENT_CLICKED, this);

        lv_obj_t *n_lbl = lv_label_create(next_btn_);
        lv_label_set_text(n_lbl, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_font(n_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(n_lbl, LV_ALIGN_CENTER, 0, 0);



        // Control Panel Overlay (Wallpaper Selection List)
        overlay_ = lv_obj_create(screen_);
        lv_obj_set_size(overlay_, 296, 160);
        lv_obj_align(overlay_, LV_ALIGN_CENTER, 0, 15);
        lv_obj_set_style_bg_color(overlay_, lv_color_hex(0x0f172a), 0);
        lv_obj_set_style_bg_opa(overlay_, LV_OPA_70, 0);
        lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_style_border_color(overlay_, lv_color_hex(0x334155), 0);
        lv_obj_set_style_border_width(overlay_, 1, 0);
        lv_obj_set_style_radius(overlay_, 10, 0);
        lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(overlay_, LV_SCROLLBAR_MODE_OFF);

        Label(overlay_, "DomOS Wallpaper & Slideshow", LV_ALIGN_TOP_LEFT, 10, 6, &lv_font_montserrat_14);

        active_lbl_ = Label(overlay_, "Active: Flash Streamed JPEG", LV_ALIGN_TOP_LEFT, 10, 24);
        lv_obj_set_style_text_color(active_lbl_, lv_color_hex(0x06b6d4), 0);

        list_ = lv_obj_create(overlay_);
        lv_obj_set_size(list_, 276, 110);
        lv_obj_align(list_, LV_ALIGN_BOTTOM_MID, 0, -4);
        lv_obj_set_style_bg_color(list_, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_border_color(list_, lv_color_hex(0x334155), 0);
        lv_obj_set_style_border_width(list_, 1, 0);
        lv_obj_set_style_radius(list_, 8, 0);
        lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(list_, 4, 0);
        lv_obj_set_style_pad_row(list_, 4, 0);
        lv_obj_clear_flag(list_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(list_, LV_SCROLLBAR_MODE_OFF);

        // Screen click shows back_btn_, prev_btn_, next_btn_ and resets 3-second auto-hide timer
        lv_obj_add_event_cb(screen_, [](lv_event_t *e) {
            auto *wp_app = static_cast<WallpaperApp *>(lv_event_get_user_data(e));
            if (wp_app != nullptr && wp_app->back_btn_ != nullptr) {
                // If click came from back/prev/next button, let button callback handle it
                lv_obj_t *target = lv_event_get_target(e);
                if (target == wp_app->back_btn_ || target == wp_app->prev_btn_ || target == wp_app->next_btn_) {
                    return;
                }

                // Any click on the image shows buttons and refreshes 3s timer
                // 1. Draw JPEG background first
                wp_app->RenderActiveSlot();
                // 2. Unhide LVGL buttons
                lv_obj_clear_flag(wp_app->back_btn_, LV_OBJ_FLAG_HIDDEN);
                if (wp_app->prev_btn_) lv_obj_clear_flag(wp_app->prev_btn_, LV_OBJ_FLAG_HIDDEN);
                if (wp_app->next_btn_) lv_obj_clear_flag(wp_app->next_btn_, LV_OBJ_FLAG_HIDDEN);
                // 3. Flush LVGL buttons on top of JPEG
                lv_refr_now(nullptr);
                wp_app->ResetAutoHideTimer();
            }
        }, LV_EVENT_CLICKED, this);





        // Gesture handling (swipe left/right)
        lv_obj_add_event_cb(screen_, [](lv_event_t *e) {
            auto *wp_app = static_cast<WallpaperApp *>(lv_event_get_user_data(e));
            lv_indev_t *indev = lv_indev_get_act();
            if (wp_app != nullptr && indev != nullptr) {
                lv_dir_t dir = lv_indev_get_gesture_dir(indev);
                if (dir == LV_DIR_LEFT) {
                    wp_app->manager_.TriggerNextSlide();
                    wp_app->RenderActiveSlot();
                    lv_refr_now(nullptr);
                } else if (dir == LV_DIR_RIGHT) {
                    wp_app->manager_.TriggerPrevSlide();
                    wp_app->RenderActiveSlot();
                    lv_refr_now(nullptr);
                }
            }
        }, LV_EVENT_GESTURE, this);

        Refresh();
    }


    void Show() override
    {
        ScreenApp::Show();
        if (back_btn_ != nullptr) lv_obj_add_flag(back_btn_, LV_OBJ_FLAG_HIDDEN);
        if (prev_btn_ != nullptr) lv_obj_add_flag(prev_btn_, LV_OBJ_FLAG_HIDDEN);
        if (next_btn_ != nullptr) lv_obj_add_flag(next_btn_, LV_OBJ_FLAG_HIDDEN);
        if (overlay_ != nullptr) lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
        // HIDE order: LVGL clears first, then JPEG covers everything
        lv_refr_now(nullptr);
        RenderActiveSlot();
    }

    uint32_t last_back_click_ms_ = 0;
    lv_obj_t *back_btn_ = nullptr;
    lv_obj_t *prev_btn_ = nullptr;
    lv_obj_t *next_btn_ = nullptr;
    lv_obj_t *overlay_ = nullptr;
    lv_obj_t *active_lbl_ = nullptr;
    lv_obj_t *list_ = nullptr;
    lv_timer_t *auto_hide_timer_ = nullptr;

    void ResetAutoHideTimer()
    {
        if (auto_hide_timer_ != nullptr) {
            lv_timer_reset(auto_hide_timer_);
            lv_timer_resume(auto_hide_timer_);
        } else {
            auto_hide_timer_ = lv_timer_create([](lv_timer_t *t) {
                auto *wp_app = static_cast<WallpaperApp *>(t->user_data);
                if (wp_app != nullptr) {
                    if (wp_app->back_btn_) lv_obj_add_flag(wp_app->back_btn_, LV_OBJ_FLAG_HIDDEN);
                    if (wp_app->prev_btn_) lv_obj_add_flag(wp_app->prev_btn_, LV_OBJ_FLAG_HIDDEN);
                    if (wp_app->next_btn_) lv_obj_add_flag(wp_app->next_btn_, LV_OBJ_FLAG_HIDDEN);
                    // HIDE order: LVGL clears button areas first, then JPEG covers everything
                    lv_refr_now(nullptr);
                    wp_app->RenderActiveSlot();
                    if (wp_app->auto_hide_timer_) lv_timer_pause(wp_app->auto_hide_timer_);
                }
            }, 3000, this);
        }
    }






private:
    void RenderActiveSlot()

    {
        const char *current_path = s_slot_filepaths[s_current_slot_idx];
        bool decoded = StreamDecodeJpegFileToPanel(current_path, manager_.Board());
        if (decoded) {
            AddSystemLog("INFO", "wallpaper", "Stream decoded %s directly to ILI9341 (0KB RGB RAM)", current_path);
        } else {
            std::string url, name;
            manager_.GetWallpaperUrl(url, name);
            if (!url.empty()) {
                if (DownloadUrlToFile(url, current_path)) {
                    StreamDecodeJpegFileToPanel(current_path, manager_.Board());
                }
            }
        }
    }

    void Refresh()
    {
        RenderActiveSlot();

        if (active_lbl_ != nullptr) {
            std::string url, name;
            manager_.GetWallpaperUrl(url, name);
            char title[96];
            std::snprintf(title, sizeof(title), "Active: %s (Slot %d)", name.empty() ? "JPEG Stream" : name.c_str(), s_current_slot_idx + 1);
            lv_label_set_text(active_lbl_, title);
        }

        if (list_ == nullptr) return;
        lv_obj_clean(list_);

        int count = 0;
        std::string url, name;
        manager_.GetWallpaperUrl(url, name);
        std::string api_url = "http://192.168.0.102:8080/api/wallpapers";

        if (url.rfind("http://", 0) == 0) {
            size_t slash = url.find('/', 7);
            if (slash != std::string::npos) {
                api_url = url.substr(0, slash) + "/api/wallpapers";
            }
        }

        esp_http_client_config_t http_cfg{};
        http_cfg.url = api_url.c_str();
        http_cfg.timeout_ms = 3000;
        esp_http_client_handle_t client = esp_http_client_init(&http_cfg);

        if (client != nullptr) {
            if (esp_http_client_open(client, 0) == ESP_OK) {
                esp_http_client_fetch_headers(client);
                char *resp_buf = static_cast<char *>(heap_caps_malloc(2048, MALLOC_CAP_8BIT));
                if (resp_buf != nullptr) {
                    std::memset(resp_buf, 0, 2048);
                    int read_bytes = esp_http_client_read(client, resp_buf, 2047);
                    if (read_bytes > 0) {
                        resp_buf[read_bytes] = '\0';
                        const char *item_pos = resp_buf;

                        while ((item_pos = std::strstr(item_pos, "{\"id\"")) != nullptr || (item_pos = std::strstr(item_pos, "\"url\":\"")) != nullptr) {
                            const char *url_ptr = std::strstr(item_pos, "\"url\":\"");
                            const char *name_ptr = std::strstr(item_pos, "\"name\":\"");
                            if (url_ptr == nullptr) break;

                            url_ptr += 7;
                            const char *end_url = std::strchr(url_ptr, '"');
                            char wp_url[256] = {};
                            if (end_url != nullptr) {
                                size_t ulen = end_url - url_ptr < sizeof(wp_url) - 1 ? end_url - url_ptr : sizeof(wp_url) - 1;
                                std::strncpy(wp_url, url_ptr, ulen);
                            }

                            char wp_name[64] = {};
                            if (name_ptr != nullptr) {
                                name_ptr += 8;
                                const char *end_name = std::strchr(name_ptr, '"');
                                if (end_name != nullptr) {
                                    size_t nlen = end_name - name_ptr < sizeof(wp_name) - 1 ? end_name - name_ptr : sizeof(wp_name) - 1;
                                    std::strncpy(wp_name, name_ptr, nlen);
                                }
                            }
                            if (wp_name[0] == '\0') {
                                std::strncpy(wp_name, "Wallpaper", sizeof(wp_name) - 1);
                            }

                            count++;
                            lv_obj_t *btn = lv_btn_create(list_);
                            lv_obj_set_size(btn, 280, 36);
                            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0f172a), 0);
                            lv_obj_set_style_radius(btn, 6, 0);
                            lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

                            lv_obj_t *icon = lv_label_create(btn);
                            lv_label_set_text(icon, LV_SYMBOL_IMAGE);
                            lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);

                            lv_obj_t *lbl = lv_label_create(btn);
                            lv_label_set_text(lbl, wp_name);
                            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 36, 0);
                            lv_obj_set_style_text_color(lbl, lv_color_hex(0x06b6d4), 0);

                            struct ClickCtx {
                                WallpaperApp *app;
                                char wp_url[256];
                                char wp_name[64];
                            };
                            auto *ctx = new ClickCtx{this, {}, {}};
                            std::strncpy(ctx->wp_url, wp_url, sizeof(ctx->wp_url) - 1);
                            std::strncpy(ctx->wp_name, wp_name, sizeof(ctx->wp_name) - 1);

                            lv_obj_add_event_cb(btn, [](lv_event_t *event) {
                                auto *ctx_data = static_cast<ClickCtx *>(lv_event_get_user_data(event));
                                if (ctx_data != nullptr && ctx_data->app != nullptr) {
                                    if (ctx_data->app->overlay_ != nullptr) {
                                        lv_obj_add_flag(ctx_data->app->overlay_, LV_OBJ_FLAG_HIDDEN);
                                        lv_refr_now(nullptr);
                                    }
                                    ctx_data->app->manager_.SetWallpaperUrl(ctx_data->wp_url, ctx_data->wp_name);
                                    AddSystemLog("INFO", "wallpaper", "Selected & rendered wallpaper: %s", ctx_data->wp_name);
                                    delete ctx_data;
                                }
                            }, LV_EVENT_CLICKED, ctx);


                            item_pos = end_url != nullptr ? end_url + 1 : url_ptr + 1;
                        }
                    }
                    heap_caps_free(resp_buf);
                }

            }
            esp_http_client_cleanup(client);
        }

        if (count == 0) {
            lv_obj_t *empty_lbl = lv_label_create(list_);
            lv_label_set_text(empty_lbl, "Webserver Database connected.\nUpload wallpapers on Dashboard!");
            lv_obj_set_style_text_color(empty_lbl, lv_color_hex(0x94a3b8), 0);
            lv_obj_set_style_text_align(empty_lbl, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(empty_lbl, LV_ALIGN_CENTER, 0, 20);
        }
    }

};






class DashboardApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "dashboard"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        Label(screen_, "Dashboard", LV_ALIGN_TOP_MID, 0, 10);
        stats_ = Label(screen_, "CPU: --  RAM: --\nWi-Fi: -- dBm  Battery: --\nTemperature: --", LV_ALIGN_TOP_LEFT, 16, 42);
        chart_ = lv_chart_create(screen_);
        lv_obj_set_size(chart_, 270, 70);
        lv_obj_align(chart_, LV_ALIGN_BOTTOM_MID, 0, -50);
        lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(chart_, 30);
        lv_chart_set_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
        series_ = lv_chart_add_series(chart_, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
        AddBackButton();
        timer_ = lv_timer_create([](lv_timer_t *timer) { static_cast<DashboardApp *>(timer->user_data)->Refresh(); }, 1000, this);
    }
    void Destroy() override
    {
        if (timer_ != nullptr) lv_timer_del(timer_);
        timer_ = nullptr;
        ScreenApp::Destroy();
    }
    void Show() override { ScreenApp::Show(); Refresh(); }

private:
    void Refresh()
    {
        const unsigned ram_kib = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024U;
        const int rssi = manager_.Wifi()->Rssi();
        char text[128];
        std::snprintf(text, sizeof(text), "CPU: task-managed\nRAM: %u KiB free\nWi-Fi: %d dBm  Battery: --\nTemperature: --", ram_kib, rssi);
        lv_label_set_text(stats_, text);
        const int32_t point = rssi == 0 ? 0 : (rssi + 100 > 100 ? 100 : rssi + 100);
        lv_chart_set_next_value(chart_, series_, point);
    }

    lv_obj_t *stats_ = nullptr;
    lv_obj_t *chart_ = nullptr;
    lv_chart_series_t *series_ = nullptr;
    lv_timer_t *timer_ = nullptr;
};

class SettingsApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "settings"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        Label(screen_, "Settings", LV_ALIGN_TOP_MID, 0, 12);
        Label(screen_, "Brightness", LV_ALIGN_CENTER, -85, -25);
        lv_obj_t *slider = lv_slider_create(screen_);
        lv_obj_set_width(slider, 150);
        lv_obj_align(slider, LV_ALIGN_CENTER, 35, -25);
        lv_slider_set_range(slider, 0, 100);
        lv_slider_set_value(slider, 75, LV_ANIM_OFF);
        lv_obj_add_event_cb(slider, [](lv_event_t *event) {
            auto *manager = static_cast<AppManager *>(lv_event_get_user_data(event));
            int val = lv_slider_get_value(static_cast<lv_obj_t *>(lv_event_get_target(event)));
            manager->Board()->SetBrightness(val);
            AddSystemLog("INFO", "display", "Brightness set to %d%%", val);
        }, LV_EVENT_VALUE_CHANGED, &manager_);
        Label(screen_, "Wallpaper: POST JPEG to http://<IP>/api/upload", LV_ALIGN_CENTER, 0, 18);
        Button(screen_, "Wi-Fi", LV_ALIGN_CENTER, -75, 55, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->Launch("wifi-setup");
        }, &manager_);
        Button(screen_, "OTA", LV_ALIGN_CENTER, 75, 55, ToOta, &manager_);
        AddBackButton();
    }
};

class WifiSetupApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "wifi-setup"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        Label(screen_, "Wi-Fi Config", LV_ALIGN_TOP_MID, 0, 4);

        // Back Button on Top Left Header (so keyboard at bottom doesn't cover it)
        lv_obj_t *back_btn = lv_btn_create(screen_);
        lv_obj_set_size(back_btn, 42, 24);
        lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 6, 4);
        lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x37474F), 0);
        lv_obj_set_style_bg_grad_color(back_btn, lv_color_hex(0x263238), 0);
        lv_obj_set_style_bg_grad_dir(back_btn, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_radius(back_btn, 6, 0);
        lv_obj_set_style_shadow_width(back_btn, 0, 0);
        lv_obj_set_style_pad_all(back_btn, 0, 0);
        lv_obj_add_event_cb(back_btn, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->CloseCurrent();
        }, LV_EVENT_CLICKED, &manager_);
        lv_obj_t *back_lbl = lv_label_create(back_btn);
        lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
        lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(back_lbl, LV_ALIGN_CENTER, 0, 0);

        ta_ssid_ = lv_textarea_create(screen_);
        lv_obj_set_size(ta_ssid_, 145, 32);
        lv_obj_align(ta_ssid_, LV_ALIGN_TOP_LEFT, 8, 30);
        lv_textarea_set_placeholder_text(ta_ssid_, "SSID");
        lv_textarea_set_one_line(ta_ssid_, true);
        if (std::strcmp(manager_.Wifi()->Ssid(), "Not configured") != 0) {
            lv_textarea_set_text(ta_ssid_, manager_.Wifi()->Ssid());
        }

        ta_pass_ = lv_textarea_create(screen_);
        lv_obj_set_size(ta_pass_, 145, 32);
        lv_obj_align(ta_pass_, LV_ALIGN_TOP_RIGHT, -8, 30);
        lv_textarea_set_placeholder_text(ta_pass_, "Password");
        lv_textarea_set_password_mode(ta_pass_, true);
        lv_textarea_set_one_line(ta_pass_, true);

        status_label_ = Label(screen_, "Tap SSID/Pass & Connect", LV_ALIGN_TOP_MID, 0, 66);

        Button(screen_, "Connect", LV_ALIGN_TOP_RIGHT, -8, 86, OnConnectClicked, this);

        kb_ = lv_keyboard_create(screen_);
        lv_obj_set_size(kb_, 315, 115);
        lv_obj_align(kb_, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(kb_, ta_ssid_);

        // When Ready (Enter/Checkmark) or Cancel button on LVGL keyboard is clicked
        lv_obj_add_event_cb(kb_, [](lv_event_t *e) {
            uint32_t btn_id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
            const char *txt = lv_btnmatrix_get_btn_text(lv_event_get_target(e), btn_id);
            if (txt != nullptr && (std::strcmp(txt, LV_SYMBOL_KEYBOARD) == 0 || std::strcmp(txt, LV_SYMBOL_CLOSE) == 0)) {
                auto *app = static_cast<WifiSetupApp *>(lv_event_get_user_data(e));
                app->manager_.CloseCurrent();
            }
        }, LV_EVENT_READY, this);
        lv_obj_add_event_cb(kb_, [](lv_event_t *e) {
            auto *app = static_cast<WifiSetupApp *>(lv_event_get_user_data(e));
            app->manager_.CloseCurrent();
        }, LV_EVENT_CANCEL, this);

        lv_obj_add_event_cb(ta_ssid_, [](lv_event_t *e) {
            auto *app = static_cast<WifiSetupApp *>(lv_event_get_user_data(e));
            lv_keyboard_set_textarea(app->kb_, app->ta_ssid_);
        }, LV_EVENT_FOCUSED, this);

        lv_obj_add_event_cb(ta_pass_, [](lv_event_t *e) {
            auto *app = static_cast<WifiSetupApp *>(lv_event_get_user_data(e));
            lv_keyboard_set_textarea(app->kb_, app->ta_pass_);
        }, LV_EVENT_FOCUSED, this);


        timer_ = lv_timer_create([](lv_timer_t *timer) { static_cast<WifiSetupApp *>(timer->user_data)->RefreshStatus(); }, 1000, this);
    }

    void Destroy() override
    {
        if (timer_ != nullptr) lv_timer_del(timer_);
        timer_ = nullptr;
        ScreenApp::Destroy();
    }

    void Show() override
    {
        ScreenApp::Show();
        RefreshStatus();
    }

private:
    static void OnConnectClicked(lv_event_t *e)
    {
        auto *app = static_cast<WifiSetupApp *>(lv_event_get_user_data(e));
        const char *ssid = lv_textarea_get_text(app->ta_ssid_);
        const char *pass = lv_textarea_get_text(app->ta_pass_);
        if (ssid != nullptr && ssid[0] != '\0') {
            app->manager_.Wifi()->ConnectTo(ssid, pass != nullptr ? pass : "");
            lv_label_set_text(app->status_label_, "Connecting to Wi-Fi...");
        } else {
            lv_label_set_text(app->status_label_, "SSID cannot be empty!");
        }
    }

    void RefreshStatus()
    {
        if (manager_.Wifi()->Connected()) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "OK: %s (%s)", manager_.Wifi()->Ssid(), manager_.Wifi()->IpAddress());
            lv_label_set_text(status_label_, buf);
        }
    }

    lv_obj_t *ta_ssid_ = nullptr;
    lv_obj_t *ta_pass_ = nullptr;
    lv_obj_t *kb_ = nullptr;
    lv_obj_t *status_label_ = nullptr;
    lv_timer_t *timer_ = nullptr;
};

class SmartHomeApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "smart-home"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        Label(screen_, "Smart Home", LV_ALIGN_TOP_MID, 0, 12);
        Button(screen_, "Lamp on", LV_ALIGN_CENTER, -75, -20, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->Mqtt()->Publish("dom/lamp", "{\"state\":\"on\"}");
        }, &manager_);
        Button(screen_, "Lamp off", LV_ALIGN_CENTER, 75, -20, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->Mqtt()->Publish("dom/lamp", "{\"state\":\"off\"}");
        }, &manager_);
        Label(screen_, "MQTT: dom/lamp", LV_ALIGN_CENTER, 0, 35);
        AddBackButton();
    }
};

class AssistantApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "assistant"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        Label(screen_, "Assistant", LV_ALIGN_TOP_MID, 0, 12);
        status_ = Label(screen_, "Voice pipeline idle", LV_ALIGN_CENTER, 0, -20);
        Button(screen_, "Hello Dom", LV_ALIGN_CENTER, 0, 25, ToggleListening, this);
        AddBackButton();
    }

private:
    static void ToggleListening(lv_event_t *event)
    {
        auto *app = static_cast<AssistantApp *>(lv_event_get_user_data(event));
        const bool listening = !app->manager_.Media()->Listening();
        app->manager_.Media()->SetListening(listening);
        lv_label_set_text(app->status_, listening ? "Listening (integration pending)" : "Voice pipeline idle");
    }

    lv_obj_t *status_ = nullptr;
};

class OtaApp final : public ScreenApp {
public:
    using ScreenApp::ScreenApp;
    const char *Id() const override { return "ota"; }
    void Create() override
    {
        screen_ = lv_obj_create(nullptr);
        Label(screen_, "OTA", LV_ALIGN_TOP_MID, 0, 12);
        Label(screen_, "HTTPS firmware update", LV_ALIGN_CENTER, 0, -20);
        Button(screen_, "Update", LV_ALIGN_CENTER, 0, 25, [](lv_event_t *event) {
            static_cast<AppManager *>(lv_event_get_user_data(event))->Ota()->UpdateFromUrl(CONFIG_DOMOS_OTA_FIRMWARE_URL);
        }, &manager_);
        AddBackButton();
    }
};
} // namespace

bool AppManager::Register(IApp *app)
{
    if (app == nullptr || app_count_ == kMaxApps) return false;
    apps_[app_count_++].app = app;
    return true;
}

bool AppManager::Start(ES3C28PBoard *board, WifiService *wifi, MqttService *mqtt, OtaService *ota,
                       MediaService *media, StorageManager *storage)
{
    board_ = board;
    wifi_ = wifi;
    mqtt_ = mqtt;
    ota_ = ota;
    media_ = media;
    storage_ = storage;
    static LauncherApp launcher(*this);
    static ClockApp clock(*this);
    static WallpaperApp wallpaper(*this);
    static DashboardApp dashboard(*this);
    static SettingsApp settings(*this);
    static WifiSetupApp wifi_setup(*this);
    static SmartHomeApp smart_home(*this);
    static AssistantApp assistant(*this);
    static OtaApp ota_app(*this);
    Register(&launcher);
    Register(&clock);
    Register(&wallpaper);
    Register(&dashboard);
    Register(&settings);
    Register(&wifi_setup);
    Register(&smart_home);
    Register(&assistant);
    Register(&ota_app);
    Launch("clock");
    return true;
}

void AppManager::Launch(const std::string &app)
{
    for (size_t index = 0; index < app_count_; ++index) {
        AppEntry &entry = apps_[index];
        if (app != entry.app->Id()) continue;
        if (current_ != nullptr && current_ != entry.app) current_->Hide();
        if (!entry.created) {
            entry.app->Create();
            entry.created = true;
        }
        current_ = entry.app;
        current_->Show();
        const unsigned free_heap_kb = esp_get_free_heap_size() / 1024U;
        ESP_LOGI("apps", "Launched '%s' [heap: %uKB free]", app.c_str(), free_heap_kb);
        AddSystemLog("INFO", "apps", "Launched '%s' [heap: %uKB free]", app.c_str(), free_heap_kb);
        return;
    }
}

void AppManager::CloseCurrent()
{
    if (current_ != nullptr && std::string(current_->Id()) != "clock") {
        AddSystemLog("INFO", "apps", "Back button pressed -> Close '%s'", current_->Id());
        current_->Hide();
    }
    Launch("clock");
}

void AppManager::ShowNextHomePage()
{
    static const char *pages[] = {"clock", "dashboard", "smart-home", "assistant"};
    size_t current_page = 0;
    for (size_t index = 0; index < sizeof(pages) / sizeof(pages[0]); ++index) {
        if (current_ != nullptr && std::string(current_->Id()) == pages[index]) current_page = index;
    }
    Launch(pages[(current_page + 1) % (sizeof(pages) / sizeof(pages[0]))]);
}

void AppManager::ApplyClockSettings(const std::string &style, uint32_t color_hex, const std::string &mode)
{
    for (size_t index = 0; index < app_count_; ++index) {
        if (std::string(apps_[index].app->Id()) == "clock") {
            auto *clock_app = static_cast<ClockApp *>(apps_[index].app);
            if (!apps_[index].created) {
                clock_app->Create();
                apps_[index].created = true;
            }
            clock_app->SetStyle(style, color_hex, mode);
            AddSystemLog("INFO", "display", "Clock style set to '%s', color: #%06X, mode: %s", style.c_str(), color_hex, mode.c_str());
            return;
        }
    }
}

void AppManager::GetClockSettings(std::string &style, uint32_t &color_hex, std::string &mode)
{
    for (size_t index = 0; index < app_count_; ++index) {
        if (std::string(apps_[index].app->Id()) == "clock") {
            auto *clock_app = static_cast<ClockApp *>(apps_[index].app);
            if (!apps_[index].created) {
                clock_app->Create();
                apps_[index].created = true;
            }
            style = clock_app->GetStyle();
            color_hex = clock_app->GetColorHex();
            mode = clock_app->GetMode();
            return;
        }
    }
    style = "digital";
    color_hex = 0x06b6d4;
    mode = "dark";
}

void AppManager::SetWallpaperUrl(const std::string &url, const std::string &name)
{
    if (!url.empty()) s_active_wallpaper_url = url;
    if (!name.empty()) s_active_wallpaper_name = name;
    AddSystemLog("INFO", "wallpaper", "Active wallpaper set to: %s (%s)", s_active_wallpaper_name.c_str(), s_active_wallpaper_url.c_str());
    if (!url.empty()) {
        const char *current_path = s_slot_filepaths[s_current_slot_idx];
        DownloadUrlToFile(url, current_path);
        if (current_ != nullptr && std::string(current_->Id()) == "wallpaper") {
            current_->Show();
        } else {
            Launch("wallpaper");
        }
    }
}



void AppManager::GetWallpaperUrl(std::string &url, std::string &name)
{
    url = s_active_wallpaper_url;
    name = s_active_wallpaper_name;
}

void AppManager::StartSlideshow(int interval_sec)
{
    if (interval_sec <= 0) interval_sec = 30;
    s_slideshow_interval_sec = interval_sec;
    s_slideshow_active = true;
    AddSystemLog("INFO", "slideshow", "Slideshow started (interval=%ds, 3-slot preloader active)", interval_sec);
    TriggerNextSlide();
}

void AppManager::StopSlideshow()
{
    s_slideshow_active = false;
    AddSystemLog("INFO", "slideshow", "Slideshow stopped");
}

bool AppManager::IsSlideshowActive() const
{
    return s_slideshow_active;
}

void AppManager::SetSlideshowInterval(int interval_sec)
{
    if (interval_sec > 0) s_slideshow_interval_sec = interval_sec;
}

int AppManager::GetSlideshowInterval() const
{
    return s_slideshow_interval_sec;
}

void AppManager::TriggerNextSlide()
{
    s_current_slot_idx = (s_current_slot_idx + 1) % 3;
    const char *current_path = s_slot_filepaths[s_current_slot_idx];
    if (board_ != nullptr) {
        StreamDecodeJpegFileToPanel(current_path, board_);
    }

    int next_slot = (s_current_slot_idx + 1) % 3;
    int next2_slot = (s_current_slot_idx + 2) % 3;
    if (!s_playlist_urls.empty()) {
        s_playlist_idx = (s_playlist_idx + 1) % s_playlist_urls.size();
        DownloadUrlToFile(s_playlist_urls[s_playlist_idx], s_slot_filepaths[next_slot]);
        size_t next2_idx = (s_playlist_idx + 1) % s_playlist_urls.size();
        DownloadUrlToFile(s_playlist_urls[next2_idx], s_slot_filepaths[next2_slot]);
    }
}

void AppManager::TriggerPrevSlide()
{
    s_current_slot_idx = (s_current_slot_idx + 2) % 3;
    const char *current_path = s_slot_filepaths[s_current_slot_idx];
    if (board_ != nullptr) {
        StreamDecodeJpegFileToPanel(current_path, board_);
    }

    int next_slot = (s_current_slot_idx + 1) % 3;
    int next2_slot = (s_current_slot_idx + 2) % 3;
    if (!s_playlist_urls.empty()) {
        s_playlist_idx = (s_playlist_idx + s_playlist_urls.size() - 1) % s_playlist_urls.size();
        DownloadUrlToFile(s_playlist_urls[s_playlist_idx], s_slot_filepaths[next_slot]);
        size_t next2_idx = (s_playlist_idx + 1) % s_playlist_urls.size();
        DownloadUrlToFile(s_playlist_urls[next2_idx], s_slot_filepaths[next2_slot]);
    }
}


