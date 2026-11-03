#include <catch2/catch_test_macros.hpp>

#include "slic3r/GUI/Gizmos/GLGizmoCut.hpp"
#include "slic3r/GUI/GLModel.hpp"
#include "slic3r/GUI/MeshUtils.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include <vector>
#include <cstddef>
#include <initializer_list>

namespace Slic3r::GUI {

// Exercise the real grouping logic without allocating OpenGL models or a wx application.
struct CutPartSelectionTestAccess {
    using PartSelection = GLGizmoCut3D::PartSelection;

    static void init(PartSelection& selection)
    {
        const TriangleMesh mesh = make_cube(1., 1., 1.);
        for (bool upper : {true, false, true, false})
            selection.m_parts.push_back({GLModel(), MeshRaycaster(mesh), upper, false, upper});
        selection.m_contour_to_parts = {{{0}, {1}}, {{2}, {3}}};
        selection.m_valid = true;
    }
};

} // namespace Slic3r::GUI

using Slic3r::GUI::CutPartSelectionTestAccess;

TEST_CASE("Initialized cut parts retain ordinary plane grouping", "[CutPartSelection][Regression]")
{
    CutPartSelectionTestAccess::PartSelection parts;
    CHECK_FALSE(parts.has_custom_assignment());
    CutPartSelectionTestAccess::init(parts);
    REQUIRE(parts.valid());
    CHECK_FALSE(parts.has_custom_assignment());
    CHECK(parts.get_ignored_contours_ptr()->empty());
}

TEST_CASE("Restoring every cut part restores ordinary plane grouping", "[CutPartSelection][Regression]")
{
    CutPartSelectionTestAccess::PartSelection parts;
    CutPartSelectionTestAccess::init(parts);
    parts.toggle_selection(0);
    REQUIRE(parts.has_custom_assignment());
    CHECK(*parts.get_ignored_contours_ptr() == std::vector<size_t>{0});
    parts.toggle_selection(2);
    parts.toggle_selection(0);
    CHECK(parts.has_custom_assignment());
    CHECK(*parts.get_ignored_contours_ptr() == std::vector<size_t>{1});
    parts.toggle_selection(2);
    CHECK_FALSE(parts.has_custom_assignment());
    CHECK(parts.get_ignored_contours_ptr()->empty());
}

TEST_CASE("Flipping a cut plane preserves deviations from default grouping", "[CutPartSelection][Regression]")
{
    CutPartSelectionTestAccess::PartSelection parts;
    CutPartSelectionTestAccess::init(parts);
    parts.turn_over_selection();
    CHECK_FALSE(parts.has_custom_assignment());
    parts.toggle_selection(0);
    parts.turn_over_selection();
    CHECK(parts.has_custom_assignment());
    parts.toggle_selection(0);
    CHECK_FALSE(parts.has_custom_assignment());
    CHECK(parts.get_ignored_contours_ptr()->empty());
}

TEST_CASE("Restored cut assignments rebuild ignored contours", "[CutPartSelection][Regression]")
{
    CutPartSelectionTestAccess::PartSelection parts;
    CutPartSelectionTestAccess::init(parts);
    const auto ordinary = parts.selection();
    parts.toggle_selection(0);
    const auto grouped = parts.selection();
    parts.set_selection(ordinary);
    CHECK_FALSE(parts.has_custom_assignment());
    CHECK(parts.get_ignored_contours_ptr()->empty());
    parts.set_selection(grouped);
    CHECK(parts.has_custom_assignment());
    CHECK(*parts.get_ignored_contours_ptr() == std::vector<size_t>{0});
}
