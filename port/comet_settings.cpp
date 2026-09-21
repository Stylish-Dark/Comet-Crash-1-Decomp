#include "comet_settings.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <fstream>
#include <sstream>

namespace {
std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}
bool parse_bool(const std::string& s, bool fallback) {
    std::string v = lower(trim(s));
    if (v=="1" || v=="true" || v=="on" || v=="yes") return true;
    if (v=="0" || v=="false" || v=="off" || v=="no") return false;
    return fallback;
}
}

void comet_settings_sanitize(CometSettings& v) {
    v.width = std::clamp(v.width, 640, 7680);
    v.height = std::clamp(v.height, 360, 4320);
    if (v.frame_limit != 0) v.frame_limit = std::clamp(v.frame_limit, 20, 360);
    v.mouse_sensitivity = std::clamp(v.mouse_sensitivity, 0.10f, 6.0f);
    v.mouse_stick = v.mouse_stick ? 1 : 0;
}

bool comet_settings_load(const std::string& path, CometSettings& out) {
    std::ifstream f(path);
    if (!f) { comet_settings_sanitize(out); return false; }
    std::string line;
    while (std::getline(f, line)) {
        auto hash = line.find_first_of("#;");
        if (hash != std::string::npos) line.resize(hash);
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = lower(trim(line.substr(0, eq)));
        std::string val = trim(line.substr(eq + 1));
        try {
            if (key == "width") out.width = std::stoi(val);
            else if (key == "height") out.height = std::stoi(val);
            else if (key == "display_mode") out.borderless = lower(val) == "borderless";
            else if (key == "borderless") out.borderless = parse_bool(val, out.borderless);
            else if (key == "vsync") out.vsync = parse_bool(val, out.vsync);
            else if (key == "frame_limit") out.frame_limit = std::stoi(val);
            else if (key == "mouse") out.mouse_enabled = parse_bool(val, out.mouse_enabled);
            else if (key == "mouse_enabled") out.mouse_enabled = parse_bool(val, out.mouse_enabled);
            else if (key == "mouse_sensitivity") out.mouse_sensitivity = std::stof(val);
            else if (key == "mouse_stick") out.mouse_stick = lower(val) == "right" ? 1 : 0;
        } catch (...) {
            // Invalid values are ignored and the previous/default value survives.
        }
    }
    comet_settings_sanitize(out);
    return true;
}

bool comet_settings_save(const std::string& path, const CometSettings& input) {
    CometSettings v = input;
    comet_settings_sanitize(v);
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;
    f << "# Comet Crash PC host settings\n";
    f << "width=" << v.width << "\n";
    f << "height=" << v.height << "\n";
    f << "display_mode=" << (v.borderless ? "borderless" : "windowed") << "\n";
    f << "vsync=" << (v.vsync ? "on" : "off") << "\n";
    f << "frame_limit=" << v.frame_limit << "\n";
    f << "mouse_enabled=" << (v.mouse_enabled ? "on" : "off") << "\n";
    f << "mouse_sensitivity=" << v.mouse_sensitivity << "\n";
    f << "mouse_stick=" << (v.mouse_stick ? "right" : "left") << "\n";
    return (bool)f;
}
