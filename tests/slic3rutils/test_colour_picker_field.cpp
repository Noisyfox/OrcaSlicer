#include "slic3r/GUI/Field.hpp"
#include "libslic3r/Config.hpp"
#include <boost/any.hpp>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <wx/colour.h>
#include <wx/string.h>

using namespace Slic3r;
using namespace Slic3r::GUI;

namespace {
class TestColourPicker : public ColourPicker {
public:
    TestColourPicker() : ColourPicker(ConfigOptionDef{}, "filament_colour") {}
    using ColourPicker::apply_user_color;
    std::string value() { return boost::any_cast<std::string>(get_value()); }
};
}

TEST_CASE("Undefined color fields remain distinct from opaque black", "[ColourPickerField]")
{
    TestColourPicker field;
    REQUIRE(field.value().empty());
    field.set_value(std::string("#000000"));
    REQUIRE(field.value() == "#000000");
    field.set_value(std::string("invalid color"));
    REQUIRE(field.value().empty());
    field.set_value(boost::any(wxString("#123456")));
    REQUIRE(field.value() == "#123456");
    field.set_value(std::string());
    REQUIRE(field.value().empty());
}

TEST_CASE("Color field assignment stays silent and explicit edits notify only changes", "[ColourPickerField]")
{
    TestColourPicker field;
    int changes = 0;
    std::string notified;
    field.m_on_change = [&](const std::string&, const boost::any& value) {
        ++changes;
        notified = boost::any_cast<std::string>(value);
    };
    field.set_value(std::string("#123456"), false);
    field.set_value(boost::any(wxString("#654321")), true);
    REQUIRE(changes == 0);
    field.apply_user_color(std::nullopt);
    REQUIRE(field.value() == "#654321");
    REQUIRE(changes == 0);
    field.apply_user_color(wxColour(0x65, 0x43, 0x21));
    REQUIRE(changes == 0);
    field.apply_user_color(wxTransparentColour);
    REQUIRE(changes == 1);
    REQUIRE(notified.empty());
    field.apply_user_color(std::nullopt);
    field.apply_user_color(wxTransparentColour);
    REQUIRE(changes == 1);
    field.apply_user_color(wxColour(0, 0, 0));
    REQUIRE(changes == 2);
    REQUIRE(notified == "#000000");
    field.apply_user_color(wxColour(0, 0, 0));
    REQUIRE(changes == 2);
    field.set_value(std::string("#123456"));
    field.m_disable_change_event = true;
    field.apply_user_color(wxColour(255, 0, 0));
    REQUIRE(field.value() == "#FF0000");
    REQUIRE(changes == 2);
}
