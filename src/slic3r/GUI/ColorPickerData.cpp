#include "ColorPickerData.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace Slic3r::GUI {
namespace {

bool normalize_color(ColorRGBA& color, bool allow_alpha)
{
    for (int i = 0; i < 4; ++i)
        if (!std::isfinite(color[i]) || color[i] < 0.0f || color[i] > 1.0f)
            return false;
    if (!allow_alpha)
        color.a(1.0f);
    return true;
}

bool parse_color(const nlohmann::json& value, ColorRGBA& color)
{
    if (!value.is_string())
        return false;
    const std::string& hex = value.get_ref<const std::string&>();
    if (!can_decode_color(hex))
        return false;
    // decode_color accepts invalid hex digits, so validate before reusing it.
    for (std::size_t i = 1; i < hex.size(); ++i) {
        const char c = hex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            return false;
    }
    return decode_color(hex, color);
}

std::string rgba_hex(const ColorRGBA& color)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::string hex(9, '#');
    for (int i = 0; i < 4; ++i) {
        const unsigned int byte = static_cast<unsigned int>(std::lround(color[i] * 255.0f));
        hex[1 + 2 * i] = digits[byte >> 4];
        hex[2 + 2 * i] = digits[byte & 15];
    }
    return hex;
}

} // namespace

std::optional<ColorSelection> normalize_color_selection(const ColorSelection& selection, ColorPickerOptions options)
{
    ColorSelection normalized = selection;
    if (auto* solid = std::get_if<ColorRGBA>(&normalized)) {
        if (!normalize_color(*solid, options.allow_alpha))
            return std::nullopt;
    } else {
        if (!options.allow_gradient)
            return std::nullopt;
        for (ColorRGBA& color : std::get<ColorGradient>(normalized))
            if (!normalize_color(color, options.allow_alpha))
                return std::nullopt;
    }
    return normalized;
}

std::optional<ColorSelection> color_selection_from_json(const nlohmann::json& value, ColorPickerOptions options)
{
    if (!value.is_object())
        return std::nullopt;
    const auto type = value.find("type");
    const auto colors = value.find("colors");
    if (type == value.end() || !type->is_string() || colors == value.end() || !colors->is_array())
        return std::nullopt;
    if (*type == "solid" && colors->size() == 1) {
        ColorRGBA solid;
        if (parse_color((*colors)[0], solid))
            return normalize_color_selection(solid, options);
    } else if (*type == "gradient" && colors->size() == 2 && options.allow_gradient) {
        ColorGradient gradient;
        if (parse_color((*colors)[0], gradient[0]) && parse_color((*colors)[1], gradient[1]))
            return normalize_color_selection(gradient, options);
    }
    return std::nullopt;
}

nlohmann::json color_selection_to_json(const ColorSelection& selection)
{
    const auto normalized = normalize_color_selection(selection, {true, true});
    if (!normalized)
        throw std::invalid_argument("Invalid color selection channels");
    if (const auto* solid = std::get_if<ColorRGBA>(&*normalized))
        return {{"type", "solid"}, {"colors", {rgba_hex(*solid)}}};
    const auto& gradient = std::get<ColorGradient>(*normalized);
    return {{"type", "gradient"}, {"colors", {rgba_hex(gradient[0]), rgba_hex(gradient[1])}}};
}

std::optional<std::vector<ColorSelection>> color_favorites_from_json(const nlohmann::json& values)
{
    if (!values.is_array() || values.size() > 24)
        return std::nullopt;
    std::vector<ColorSelection> favorites;
    favorites.reserve(values.size());
    for (const auto& value : values) {
        const auto selection = color_selection_from_json(value, {true, true});
        if (!selection)
            return std::nullopt;
        favorites.push_back(*selection);
    }
    return favorites;
}

} // namespace Slic3r::GUI
