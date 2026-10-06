#include "FilamentColorPicker.hpp"
#include "ColorPickerData.hpp"
#include <cstddef>
#include <optional>

#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"

#include <sstream>
#include <utility>
#include <nlohmann/json.hpp>

namespace Slic3r::GUI {

FilamentColorPickerValue filament_color_picker_value(const DynamicPrintConfig& config, std::size_t index)
{
    FilamentColorPickerValue value;
    if (const auto* primary = config.opt<ConfigOptionStrings>("filament_colour"); primary && index < primary->values.size())
        value.primary = primary->values[index];
    if (const auto* multi = config.opt<ConfigOptionStrings>("filament_multi_colour"); multi && index < multi->values.size()) {
        std::istringstream stream(multi->values[index]);
        for (std::string color; stream >> color;)
            value.colors.push_back(std::move(color));
    }
    if (value.colors.empty() && !value.primary.empty())
        value.colors.push_back(value.primary);
    if (const auto* types = config.opt<ConfigOptionStrings>("filament_colour_type"); types && index < types->values.size())
        value.gradient = value.colors.size() > 1 && types->values[index] == "0";
    return value;
}

std::optional<ColorSelection> filament_color_picker_selection(const FilamentColorPickerValue& value)
{
    return color_selection_from_json({{"type", value.gradient ? "gradient" : "solid"}, {"colors", value.colors}}, {true, true});
}

ColorSelection filament_color_picker_initial(const FilamentColorPickerValue& value)
{
    if (const auto exact = filament_color_picker_selection(value))
        return *exact;
    if (!value.colors.empty()) {
        if (const auto primary = color_selection_from_json({{"type", "solid"}, {"colors", {value.colors.front()}}}, {false, true}))
            return *primary;
    }
    if (const auto primary = color_selection_from_json({{"type", "solid"}, {"colors", {value.primary}}}, {false, true}))
        return *primary;
    return ColorRGBA(0.f, 0.f, 0.f, 1.f);
}

FilamentColorPickerValue filament_color_picker_result(const ColorSelection& selection)
{
    // Keep legacy RGB spelling for opaque colors; retain RGBA for transparency.
    const auto canonical = color_selection_to_json(selection);
    FilamentColorPickerValue value;
    value.gradient = canonical.at("type") == "gradient";
    for (const auto& color : canonical.at("colors")) {
        auto hex = color.get<std::string>();
        if (hex.compare(7, 2, "FF") == 0) hex.resize(7);
        value.colors.push_back(std::move(hex));
    }
    return value;
}

bool filament_color_picker_changed(const ColorSelection& initial, const std::optional<ColorSelection>& result)
{
    // Compare encoded bytes, not ColorRGBA's tolerant floating point equality.
    // An unchanged seed also leaves unrepresentable multi-color data untouched.
    return result && color_selection_to_json(initial) != color_selection_to_json(*result);
}

} // namespace Slic3r::GUI
