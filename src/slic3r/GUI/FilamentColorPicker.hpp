#pragma once

#include "ColorPickerData.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Slic3r { class DynamicPrintConfig; }

namespace Slic3r::GUI {

// Project storage preserves direction; FilamentColor's set is only for the
// official palette and cannot represent ordered custom gradients.
struct FilamentColorPickerValue {
    std::vector<std::string> colors;
    bool gradient = false;
    std::string primary;
};

FilamentColorPickerValue filament_color_picker_value(const DynamicPrintConfig& config, std::size_t index);
std::optional<ColorSelection> filament_color_picker_selection(const FilamentColorPickerValue& value);
ColorSelection filament_color_picker_initial(const FilamentColorPickerValue& value);
FilamentColorPickerValue filament_color_picker_result(const ColorSelection& selection);
bool filament_color_picker_changed(const ColorSelection& initial, const std::optional<ColorSelection>& result);

} // namespace Slic3r::GUI
