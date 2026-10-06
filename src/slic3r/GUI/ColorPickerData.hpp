#pragma once

#include <array>
#include <optional>
#include <variant>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "libslic3r/Color.hpp"

namespace Slic3r::GUI {

// Gradient endpoints retain their order; a solid has exactly one color.
using ColorGradient = std::array<ColorRGBA, 2>;
using ColorSelection = std::variant<ColorRGBA, ColorGradient>;

struct ColorPickerOptions {
    bool allow_gradient = false;
    bool allow_alpha = false;
};

// Reject invalid channels and unsupported gradients. Disabled alpha becomes opaque.
// Returns a copy so previews and failed/cancelled dialogs cannot modify caller state.
std::optional<ColorSelection> normalize_color_selection(const ColorSelection& selection, ColorPickerOptions options = {});

// Bridge shape: {"type":"solid"|"gradient", "colors":["#RRGGBB[AA]", ...]}.
// Parsing rejects malformed hex/type/cardinality and applies native capabilities.
std::optional<ColorSelection> color_selection_from_json(const nlohmann::json& value, ColorPickerOptions options = {});

// Canonical uppercase RGBA hex, including FF for opaque colors. Throws
// std::invalid_argument for invalid native channels; never drops endpoint alpha.
nlohmann::json color_selection_to_json(const ColorSelection& selection);

// Favorites always retain full RGBA/gradient capabilities. Reject an invalid
// collection as a whole, including collections exceeding the 24-slot limit.
std::optional<std::vector<ColorSelection>> color_favorites_from_json(const nlohmann::json& values);

} // namespace Slic3r::GUI
