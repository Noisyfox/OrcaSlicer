#include <limits>
#include <stdexcept>
#include <string>
#include <variant>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>

#include "slic3r/GUI/ColorPickerDialog.hpp"
#include "libslic3r/Color.hpp"

using namespace Slic3r;
using namespace Slic3r::GUI;
using nlohmann::json;

TEST_CASE("Solid selections accept RGB and preserve RGBA when enabled", "[ColorPickerData]")
{
    const std::string hex = GENERATE("#1234ab", "#1234ab80", "#1234ab00", "#1234abFF");
    const auto selection = color_selection_from_json({{"type", "solid"}, {"colors", {hex}}}, {false, true});
    REQUIRE(selection.has_value());
    REQUIRE(std::holds_alternative<ColorRGBA>(*selection));
    const std::string expected = hex.size() == 7 ? "#1234ABFF" : "#1234AB" + hex.substr(7);
    REQUIRE(color_selection_to_json(*selection)["colors"][0] == expected);
}

TEST_CASE("Gradient endpoints retain their order and independent alpha", "[ColorPickerData]")
{
    const json value = {{"type", "gradient"}, {"colors", {"#FF000020", "#0000FF80"}}};
    const auto selection = color_selection_from_json(value, {true, true});
    REQUIRE(selection.has_value());
    REQUIRE(std::holds_alternative<ColorGradient>(*selection));
    REQUIRE(color_selection_to_json(*selection) == value);
    const json reversed = {{"type", "gradient"}, {"colors", {"#0000FF80", "#FF000020"}}};
    const auto reversed_selection = color_selection_from_json(reversed, {true, true});
    REQUIRE(reversed_selection.has_value());
    REQUIRE(color_selection_to_json(*reversed_selection) == reversed);
}

TEST_CASE("Gradient and alpha capabilities default off and remain independent", "[ColorPickerData]")
{
    const ColorPickerOptions defaults;
    REQUIRE_FALSE(defaults.allow_gradient);
    REQUIRE_FALSE(defaults.allow_alpha);
    const json gradient = {{"type", "gradient"}, {"colors", {"#FF000000", "#0000FF80"}}};
    REQUIRE_FALSE(color_selection_from_json(gradient).has_value());
    REQUIRE_FALSE(color_selection_from_json(gradient, {false, true}).has_value());
    const auto opaque = color_selection_from_json(gradient, {true, false});
    REQUIRE(opaque.has_value());
    REQUIRE(color_selection_to_json(*opaque)["colors"] == json::array({"#FF0000FF", "#0000FFFF"}));
    const auto solid = color_selection_from_json({{"type", "solid"}, {"colors", {"#ABCDEF00"}}});
    REQUIRE(solid.has_value());
    REQUIRE(color_selection_to_json(*solid)["colors"][0] == "#ABCDEFFF");
}

TEST_CASE("Malformed bridge selections are rejected", "[ColorPickerData]")
{
    const json value = GENERATE(
        json(), json::array(), json::object(),
        json({{"type", 1}, {"colors", {"#112233"}}}),
        json({{"type", "solid"}, {"colors", "#112233"}}),
        json({{"type", "solid"}, {"colors", json::array()}}),
        json({{"type", "solid"}, {"colors", {"#112233", "#445566"}}}),
        json({{"type", "gradient"}, {"colors", {"#112233"}}}),
        json({{"type", "gradient"}, {"colors", {"#112233", "#445566", "#778899"}}}),
        json({{"type", "other"}, {"colors", {"#112233"}}}),
        json({{"type", "solid"}, {"colors", {nullptr}}}));
    REQUIRE_FALSE(color_selection_from_json(value, {true, true}).has_value());
}

TEST_CASE("Bridge colors require complete hexadecimal RGB or RGBA", "[ColorPickerData]")
{
    const std::string hex = GENERATE("", "112233", "#ABC", "#1234567", "#123456789", "#GG2233", "#11223Z", "#112233G0", " #112233", "#112233 ");
    REQUIRE_FALSE(color_selection_from_json({{"type", "solid"}, {"colors", {hex}}}, {true, true}).has_value());
    REQUIRE_FALSE(color_selection_from_json({{"type", "gradient"}, {"colors", {"#112233", hex}}}, {true, true}).has_value());
}

TEST_CASE("Normalization leaves the input untouched and rejects invalid native channels", "[ColorPickerData]")
{
    ColorRGBA translucent = ColorRGBA::RED();
    translucent.a(0.0f);
    const ColorSelection input = translucent;
    const auto normalized = normalize_color_selection(input);
    REQUIRE(normalized.has_value());
    REQUIRE(color_selection_to_json(input)["colors"][0] == "#FF000000");
    REQUIRE(color_selection_to_json(*normalized)["colors"][0] == "#FF0000FF");

    const float invalid = GENERATE(-0.1f, 1.1f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN());
    const int channel = GENERATE(0, 1, 2, 3);
    ColorRGBA color;
    color[channel] = invalid;
    REQUIRE_FALSE(normalize_color_selection(color, {true, true}).has_value());
    REQUIRE_FALSE(normalize_color_selection(ColorGradient{ColorRGBA::WHITE(), color}, {true, true}).has_value());
    REQUIRE_THROWS_AS(color_selection_to_json(color), std::invalid_argument);
}

TEST_CASE("Every hexadecimal byte survives a bridge round trip", "[ColorPickerData]")
{
    constexpr char digits[] = "0123456789ABCDEF";
    for (int byte = 0; byte < 256; ++byte) {
        std::string hex = "#";
        for (int channel = 0; channel < 4; ++channel) {
            hex += digits[byte >> 4];
            hex += digits[byte & 15];
        }
        const json value = {{"type", "solid"}, {"colors", {hex}}};
        const auto selection = color_selection_from_json(value, {false, true});
        REQUIRE(selection.has_value());
        REQUIRE(color_selection_to_json(*selection) == value);
    }
}

TEST_CASE("Favorite bridge collections reject malformed entries atomically and retain full capabilities", "[ColorPickerData]")
{
    const json solid = {{"type", "solid"}, {"colors", {"#ABCDEF20"}}};
    const json gradient = {{"type", "gradient"}, {"colors", {"#FF000000", "#0000FF80"}}};
    const auto favorites = color_favorites_from_json(json::array({solid, gradient}));
    REQUIRE(favorites.has_value());
    REQUIRE(favorites->size() == 2);
    REQUIRE(color_selection_to_json((*favorites)[0]) == solid);
    REQUIRE(color_selection_to_json((*favorites)[1]) == gradient);
    const auto empty = color_favorites_from_json(json::array());
    REQUIRE(empty.has_value());
    REQUIRE(empty->empty());
    REQUIRE_FALSE(color_favorites_from_json(json::object()).has_value());
    REQUIRE_FALSE(color_favorites_from_json(json::array({solid, nullptr})).has_value());
    json too_many = json::array();
    for (int i = 0; i < 25; ++i)
        too_many.push_back(solid);
    REQUIRE_FALSE(color_favorites_from_json(too_many).has_value());
}
