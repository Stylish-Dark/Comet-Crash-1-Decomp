#pragma once
#include <string>

struct CometSettings {
    int width = 1280;
    int height = 720;
    bool borderless = false;
    bool vsync = true;
    int frame_limit = 60;        // 0 = uncapped host presentation loop
    bool mouse_enabled = true;
    float mouse_sensitivity = 1.0f;
    int mouse_stick = 0;         // 0 = left, 1 = right
};

bool comet_settings_load(const std::string& path, CometSettings& out);
bool comet_settings_save(const std::string& path, const CometSettings& value);
void comet_settings_sanitize(CometSettings& value);
