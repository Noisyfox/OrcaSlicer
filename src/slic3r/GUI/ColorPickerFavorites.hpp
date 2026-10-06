#pragma once

#include "ColorPickerData.hpp"

#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace Slic3r { class AppConfig; }

namespace Slic3r::GUI {

struct ColorPickerFavoritesState {
    std::vector<ColorSelection> favorites;
    bool writable = true;
};

// Imports legacy solid colors once when the independent section is absent. Unknown
// versions remain untouched and read-only. Invalid v1 data is not auto-rewritten.
ColorPickerFavoritesState load_color_picker_favorites(AppConfig& config);

// Validates, normalizes, deduplicates and replaces the entire v1 section, then
// attempts AppConfig::save immediately. Returns false for invalid data/version.
bool save_color_picker_favorites(AppConfig& config, const nlohmann::json& values);

} // namespace Slic3r::GUI
