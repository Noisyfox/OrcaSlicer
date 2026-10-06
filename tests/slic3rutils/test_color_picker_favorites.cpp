#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Thread.hpp"
#include "plugin_test_utils.hpp"
#include "slic3r/GUI/ColorPickerFavorites.hpp"
#include "slic3r/GUI/ColorPickerData.hpp"
#include "slic3r/Utils/ColorSpaceConvert.hpp"
#include <wx/colour.h>

using namespace Slic3r;
using namespace Slic3r::GUI;
using nlohmann::json;

namespace {
json serialized(const ColorPickerFavoritesState& state)
{
    json values = json::array();
    for (const auto& selection : state.favorites)
        values.push_back(color_selection_to_json(selection));
    return values;
}
}

TEST_CASE("Favorite replacement survives restart and preserves RGBA endpoint order", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker");
    save_main_thread_id();
    AppConfig config;
    config.set_section("color_picker", {{"version", "1"}, {"favorites", "[]"}, {"old_slot", "obsolete"}});
    config.set("unrelated", "key", "kept");
    const json gradient = {{"type", "gradient"}, {"colors", {"#FF000001", "#0000FF80"}}};
    const json reversed = {{"type", "gradient"}, {"colors", {"#0000FF80", "#FF000001"}}};
    const json values = json::array({{{"type", "solid"}, {"colors", {"#abcdef"}}},
                                     {{"type", "solid"}, {"colors", {"#ABCDEFFF"}}}, gradient, reversed});
    REQUIRE(save_color_picker_favorites(config, values));
    REQUIRE_FALSE(config.has("color_picker", "old_slot"));
    AppConfig restarted;
    REQUIRE(restarted.load().empty());
    const auto loaded = load_color_picker_favorites(restarted);
    REQUIRE(loaded.writable);
    REQUIRE(serialized(loaded) == json::array({{{"type", "solid"}, {"colors", {"#ABCDEFFF"}}}, gradient, reversed}));
    REQUIRE(restarted.get("unrelated", "key") == "kept");
}

TEST_CASE("Legacy color import runs once and clearing remains empty across restart", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker-import");
    save_main_thread_id();
    AppConfig config;
    const std::map<std::string, std::string> legacy = {{"10", "#ff0000"}, {"11", "#FF0000"}, {"12", "#00FF00"},
        {"13", "#GG2233"}, {"14", "#ABCDEF80"}, {"15", "#112233-#445566"}, {"16", "#ABC"}};
    config.set_section("custom_color_list", legacy);
    const auto imported = load_color_picker_favorites(config);
    REQUIRE(serialized(imported) == json::array({{{"type", "solid"}, {"colors", {"#FF0000FF"}}},
                                                {{"type", "solid"}, {"colors", {"#00FF00FF"}}},
                                                {{"type", "solid"}, {"colors", {"#ABCDEF80"}}}}));
    REQUIRE(config.get_section("custom_color_list") == legacy);
    AppConfig restarted;
    REQUIRE(restarted.load().empty());
    REQUIRE(serialized(load_color_picker_favorites(restarted)) == serialized(imported));
    REQUIRE(restarted.get_section("custom_color_list") == legacy);
    REQUIRE(save_color_picker_favorites(restarted, json::array()));
    AppConfig cleared;
    REQUIRE(cleared.load().empty());
    REQUIRE(load_color_picker_favorites(cleared).favorites.empty());
    REQUIRE(cleared.get("color_picker", "version") == "1");
    REQUIRE(cleared.get_section("custom_color_list") == legacy);
}

TEST_CASE("Actual native custom color strings migrate strictly with alpha preserved", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker-native-import");
    save_main_thread_id();
    AppConfig config;
    const std::vector<std::string> legacy = {color_to_string(wxColour(12, 34, 56, 78)),
        color_to_string(wxColour(255, 0, 0, 255)), "#ff0000", "#11223344", "0,0,0,0", "255,255,255,255",
        "1,2,3", "1,2,3,4,5", "1,2,3,256", "-1,2,3,4", "1,2,3,4junk", "1.5,2,3,4", "1,2,,4",
        "1,2,3,", "+1,2,3,4", "1,2,3, 4", "999999999999,2,3,4", "#GG2233", "#112233-#445566"};
    config.save_custom_color_to_config(legacy);
    config.set_dirty();
    config.save();
    AppConfig restarted;
    REQUIRE(restarted.load().empty());
    const json expected = json::array({{{"type", "solid"}, {"colors", {"#0C22384E"}}},
        {{"type", "solid"}, {"colors", {"#FF0000FF"}}}, {{"type", "solid"}, {"colors", {"#11223344"}}},
        {{"type", "solid"}, {"colors", {"#00000000"}}}, {{"type", "solid"}, {"colors", {"#FFFFFFFF"}}}});
    REQUIRE(serialized(load_color_picker_favorites(restarted)) == expected);
    REQUIRE(restarted.get_custom_color_from_config() == legacy);
    AppConfig migrated;
    REQUIRE(migrated.load().empty());
    REQUIRE(serialized(load_color_picker_favorites(migrated)) == expected);
}

TEST_CASE("An empty first import persists independently of later legacy changes", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker-empty");
    save_main_thread_id();
    AppConfig config;
    REQUIRE(load_color_picker_favorites(config).favorites.empty());
    config.set("custom_color_list", "10", "#112233");
    config.save();
    AppConfig restarted;
    REQUIRE(restarted.load().empty());
    REQUIRE(load_color_picker_favorites(restarted).favorites.empty());
}

TEST_CASE("Unknown or missing favorite versions remain untouched and read only", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker-version");
    save_main_thread_id();
    AppConfig config;
    const std::string version = GENERATE("2", "garbage", "");
    std::map<std::string, std::string> section = {{"favorites", "future-data"}, {"future_key", "kept"}};
    if (!version.empty())
        section["version"] = version;
    config.set_section("color_picker", section);
    config.set("custom_color_list", "10", "#112233");
    config.save();
    AppConfig restarted;
    REQUIRE(restarted.load().empty());
    const auto loaded = load_color_picker_favorites(restarted);
    REQUIRE_FALSE(loaded.writable);
    REQUIRE(loaded.favorites.empty());
    REQUIRE_FALSE(save_color_picker_favorites(restarted, json::array()));
    REQUIRE(restarted.get_section("color_picker") == section);
}

TEST_CASE("Invalid favorite data is left intact until an explicit valid replacement", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker-invalid");
    save_main_thread_id();
    AppConfig config;
    const std::string malformed = GENERATE("{broken", "{}", "[null]");
    config.set_section("color_picker", {{"version", "1"}, {"favorites", malformed}});
    config.set("custom_color_list", "10", "#112233");
    const auto loaded = load_color_picker_favorites(config);
    REQUIRE(loaded.writable);
    REQUIRE(loaded.favorites.empty());
    REQUIRE(config.get("color_picker", "favorites") == malformed);
    REQUIRE_FALSE(save_color_picker_favorites(config, json::array({nullptr})));
    REQUIRE(config.get("color_picker", "favorites") == malformed);
    const json replacement = json::array({{{"type", "solid"}, {"colors", {"#ABCDEF00"}}}});
    REQUIRE(save_color_picker_favorites(config, replacement));
    AppConfig restarted;
    REQUIRE(restarted.load().empty());
    REQUIRE(serialized(load_color_picker_favorites(restarted)) == replacement);
}

TEST_CASE("Legacy imports stop at twenty four distinct favorites", "[ColorPickerFavorites]")
{
    ScopedDataDir data("color-picker-limit");
    save_main_thread_id();
    AppConfig config;
    for (int i = 0; i < 30; ++i) {
        const auto hex = color_selection_to_json(ColorRGBA(float(i) / 255.0f, 0.0f, 0.0f, 1.0f))["colors"][0].get<std::string>();
        config.set("custom_color_list", std::to_string(10 + i), hex.substr(0, 7));
    }
    const auto loaded = load_color_picker_favorites(config);
    REQUIRE(loaded.favorites.size() == 24);
    REQUIRE(config.get_section("custom_color_list").size() == 30);
    json too_many = serialized(loaded);
    too_many.push_back(too_many.front());
    REQUIRE_FALSE(save_color_picker_favorites(config, too_many));
    REQUIRE(load_color_picker_favorites(config).favorites.size() == 24);
}
