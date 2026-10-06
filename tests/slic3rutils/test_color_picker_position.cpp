#include "slic3r/GUI/ColorPickerDialog.hpp"
#include <wx/gdicmn.h>
#include <catch2/catch_test_macros.hpp>

using Slic3r::GUI::color_picker_panel_position;

TEST_CASE("Color panels align below their trigger and flip above when content grows", "[ColorPickerPosition]")
{
    const wxRect area(0, 0, 1920, 1080), anchor(100, 600, 24, 24);
    REQUIRE(color_picker_panel_position(anchor, area, wxSize(550, 300), 4) == wxPoint(100, 628));
    REQUIRE(color_picker_panel_position(anchor, area, wxSize(550, 529), 4) == wxPoint(100, 67));
    REQUIRE(color_picker_panel_position(anchor, area, wxSize(550, 300), 4) == wxPoint(100, 628));
}

TEST_CASE("Color panels clamp to the anchor display including negative monitor coordinates", "[ColorPickerPosition]")
{
    const wxRect left_display(-1920, -200, 1920, 1040);
    REQUIRE(color_picker_panel_position(wxRect(-100, 700, 24, 24), left_display, wxSize(550, 529), 4) == wxPoint(-550, 167));
    REQUIRE(color_picker_panel_position(wxRect(-2000, -250, 24, 24), left_display, wxSize(550, 529), 4) == wxPoint(-1920, -200));
    REQUIRE(color_picker_panel_position(wxRect(-100, 400, 24, 24), left_display, wxSize(550, 1000), 4) == wxPoint(-550, -200));
    const wxRect above_display(0, -1440, 2560, 1400);
    REQUIRE(color_picker_panel_position(wxRect(2500, -100, 48, 48), above_display, wxSize(825, 794), 6) == wxPoint(1735, -900));
}

TEST_CASE("Color panel geometry preserves device-scaled trigger spacing", "[ColorPickerPosition]")
{
    const wxRect area(0, 0, 1920, 1080), anchor(100, 100, 24, 24);
    const auto normal = color_picker_panel_position(anchor, area, wxSize(550, 300), 4);
    const auto scaled = color_picker_panel_position(wxRect(150, 150, 36, 36), wxRect(0, 0, 2880, 1620), wxSize(825, 450), 6);
    REQUIRE(scaled == wxPoint(normal.x * 3 / 2, normal.y * 3 / 2));
}
