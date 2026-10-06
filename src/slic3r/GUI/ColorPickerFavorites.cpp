#include "ColorPickerFavorites.hpp"
#include "ColorPickerData.hpp"

#include "libslic3r/AppConfig.hpp"

#include <map>
#include <array>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <nlohmann/json.hpp>

namespace Slic3r::GUI {
namespace {
std::optional<ColorSelection> legacy_color(const std::string& value)
{
    if (!value.empty() && value.front() == '#')
        return color_selection_from_json({{"type", "solid"}, {"colors", {value}}}, {false, true});

    // string_to_wxColor uses stoi without checking the consumed length or byte
    // range. Parse the actual color_to_string RGBA format strictly for migration.
    std::array<unsigned int, 4> bytes;
    std::size_t begin = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto comma = value.find(',', begin);
        if ((i < 3 && comma == std::string::npos) || (i == 3 && comma != std::string::npos))
            return std::nullopt;
        const auto end = comma == std::string::npos ? value.size() : comma;
        const auto parsed = std::from_chars(value.data() + begin, value.data() + end, bytes[i]);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + end || bytes[i] > 255)
            return std::nullopt;
        begin = end + 1;
    }
    return ColorRGBA(static_cast<unsigned char>(bytes[0]), static_cast<unsigned char>(bytes[1]),
                     static_cast<unsigned char>(bytes[2]), static_cast<unsigned char>(bytes[3]));
}
} // namespace

bool save_color_picker_favorites(AppConfig& config, const nlohmann::json& values)
{
    if (config.has_section("color_picker") && config.get("color_picker", "version") != "1")
        return false;
    const auto favorites = color_favorites_from_json(values);
    if (!favorites)
        return false;
    nlohmann::json canonical = nlohmann::json::array();
    for (const auto& selection : *favorites)
        canonical.push_back(color_selection_to_json(selection));
    // set_section is a raw replacement and does not mark AppConfig dirty itself.
    config.set_section("color_picker", {{"version", "1"}, {"favorites", canonical.dump()}});
    config.set_dirty();
    config.save();
    return true;
}

ColorPickerFavoritesState load_color_picker_favorites(AppConfig& config)
{
    if (config.has_section("color_picker")) {
        if (config.get("color_picker", "version") != "1")
            return {{}, false};
        const auto values = nlohmann::json::parse(config.get("color_picker", "favorites"), nullptr, false);
        const auto favorites = color_favorites_from_json(values);
        return {favorites ? *favorites : std::vector<ColorSelection>{}, true};
    }

    nlohmann::json imported = nlohmann::json::array();
    for (const std::string& value_string : config.get_custom_color_from_config()) {
        const auto selection = legacy_color(value_string);
        if (!selection)
            continue;
        const auto value = color_selection_to_json(*selection);
        bool duplicate = false;
        for (const auto& existing : imported)
            if (existing == value) {
                duplicate = true;
                break;
            }
        if (!duplicate)
            imported.push_back(value);
        if (imported.size() == 24)
            break;
    }
    // Persist the version even for an empty import: clearing never reimports.
    save_color_picker_favorites(config, imported);
    return {*color_favorites_from_json(imported), true};
}

} // namespace Slic3r::GUI
