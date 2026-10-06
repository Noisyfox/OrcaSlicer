#include "slic3r/GUI/FilamentColorPicker.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/PrintConfig.hpp"

#include <optional>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>

using namespace Slic3r;
using namespace Slic3r::GUI;

TEST_CASE("Project gradient direction survives editing and RGB serialization", "[FilamentColorPicker]")
{
    DynamicPrintConfig config;
    const std::vector<std::string> endpoints = {"#FF0000", "#0000FF"};
    config.set_key_value("filament_colour", new ConfigOptionStrings({endpoints.front()}));
    config.set_key_value("filament_multi_colour", new ConfigOptionStrings({endpoints[0] + " " + endpoints[1]}));
    config.set_key_value("filament_colour_type", new ConfigOptionStrings({"0"}));
    const auto stored = filament_color_picker_value(config, 0);
    REQUIRE(stored.colors == endpoints);
    REQUIRE(stored.gradient);
    const auto initial = filament_color_picker_selection(stored);
    REQUIRE(initial);
    REQUIRE(filament_color_picker_result(*initial).colors == endpoints);
    REQUIRE_FALSE(filament_color_picker_changed(*initial, initial));
    REQUIRE_FALSE(filament_color_picker_changed(*initial, std::nullopt));
    const auto reverse = filament_color_picker_selection({{endpoints[1], endpoints[0]}, true});
    REQUIRE(filament_color_picker_changed(*initial, reverse));
    REQUIRE(filament_color_picker_result(*reverse).colors.front() == endpoints.back());
}

TEST_CASE("Unrepresentable project colors remain unchanged until a different selection is confirmed", "[FilamentColorPicker]")
{
    const auto value = GENERATE(FilamentColorPickerValue{{"#FF0000", "#00FF00", "#0000FF"}, true},
                                FilamentColorPickerValue{{"#FF0000", "#0000FF"}, false});
    REQUIRE_FALSE(filament_color_picker_selection(value));
    const auto seed = filament_color_picker_initial(value);
    REQUIRE_FALSE(filament_color_picker_changed(seed, seed));
    REQUIRE_FALSE(filament_color_picker_changed(seed, std::nullopt));
    const auto changed = filament_color_picker_selection({{"#FE0000"}, false});
    REQUIRE(filament_color_picker_changed(seed, changed));
    REQUIRE(filament_color_picker_result(*changed).colors == std::vector<std::string>{"#FE0000"});
}

TEST_CASE("Filament selections normalize alpha and fall back safely when project arrays are missing", "[FilamentColorPicker]")
{
    DynamicPrintConfig config;
    config.set_key_value("filament_colour", new ConfigOptionStrings({"#123456"}));
    REQUIRE(filament_color_picker_value(config, 0).colors == std::vector<std::string>{"#123456"});
    config.set_key_value("filament_multi_colour", new ConfigOptionStrings({"invalid #00FF00"}));
    const auto stale = filament_color_picker_value(config, 0);
    REQUIRE_FALSE(filament_color_picker_selection(stale));
    REQUIRE(filament_color_picker_result(filament_color_picker_initial(stale)).colors == std::vector<std::string>{"#123456"});
    const auto value = filament_color_picker_selection({{"#12345680", "#ABCDEF01"}, true});
    REQUIRE(value);
    REQUIRE(color_selection_to_json(*value).at("colors")[0] == "#123456FF");
    REQUIRE(filament_color_picker_result(*value).colors == std::vector<std::string>{"#123456", "#ABCDEF"});
    const auto missing = filament_color_picker_initial(filament_color_picker_value(config, 3));
    REQUIRE(filament_color_picker_result(missing).colors == std::vector<std::string>{"#000000"});
}
