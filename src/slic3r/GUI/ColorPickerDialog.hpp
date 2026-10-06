#pragma once

#include "libslic3r/Color.hpp"
#include "Widgets/WebViewHostDialog.hpp"

#include <array>
#include <cstddef>
#include <variant>
#include <nlohmann/json_fwd.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <wx/gdicmn.h>
#include <wx/bitmap.h>

class wxWindow;
namespace Slic3r { class AppConfig; class DynamicPrintConfig; }

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

// Favorites retain full RGBA/gradient capabilities and deduplicate canonical
// values in order. Reject invalid collections or inputs exceeding 24 slots.
std::optional<std::vector<ColorSelection>> color_favorites_from_json(const nlohmann::json& values);

struct ColorPickerFavoritesState {
    std::vector<ColorSelection> favorites;
    bool writable = true;
};

// Imports legacy solid colors once when the independent section is absent. Unknown
// versions remain untouched and read-only. Invalid v1 data is not auto-rewritten.
ColorPickerFavoritesState load_color_picker_favorites(AppConfig& config);

// Validates, normalizes, deduplicates and replaces the entire v1 section, then
// attempts AppConfig::save immediately. Returns false for invalid data/version.
bool save_color_picker_favorites(AppConfig& config, const nlohmann::json& values);

// Project storage preserves direction; FilamentColor's set is only for the
// official palette and cannot represent ordered custom gradients.
struct FilamentColorPickerValue {
    std::vector<std::string> colors;
    bool gradient = false;
    std::string primary;
};

FilamentColorPickerValue filament_color_picker_value(const DynamicPrintConfig& config, std::size_t index);
std::optional<ColorSelection> filament_color_picker_selection(const FilamentColorPickerValue& value);
ColorSelection filament_color_picker_initial(const FilamentColorPickerValue& value);
FilamentColorPickerValue filament_color_picker_result(const ColorSelection& selection);
bool filament_color_picker_changed(const ColorSelection& initial, const std::optional<ColorSelection>& result);

// All geometry uses native screen coordinates, including negative monitor origins.
wxPoint color_picker_panel_position(const wxRect& anchor, const wxRect& work_area, const wxSize& size, int gap);

class ColorPickerDialog : public WebViewHostDialog
{
public:
    ColorPickerDialog(wxWindow* parent, const ColorSelection& initial, ColorPickerOptions options = {},
                      bool preserve_multi_color = false, wxWindow* anchor = nullptr);
    ~ColorPickerDialog() override;

    bool is_available() const { return m_available; }
    const std::optional<ColorSelection>& selection() const { return m_selection; }

private:
    void add_user_scripts() override;
    void on_script_message(const nlohmann::json& payload) override;
    void handle_web_command(const nlohmann::json& payload);
    void send_initial_state();
    void finish(int return_code);
    void apply_rounded_shape();
    void position_panel();
    void resize_to_content(int height);
    void focus_webview();
    void repaint_webview();
    void on_dpi_changed(const wxRect& suggested_rect) override;

    ColorSelection m_initial;
    ColorPickerOptions m_options;
    std::optional<ColorSelection> m_selection;
    std::vector<ColorSelection> m_favorites;
    std::string m_page_id;
    bool m_preserve_multi_color = false;
    bool m_available = false;
    bool m_init_sent = false;
    bool m_page_ready = false;
    bool m_closing = false;
    bool m_favorites_writable = false;
    int m_corner_radius{8};
    wxBitmap m_shape_bmp;
    bool m_applying_shape{false};
    wxRect m_anchor_rect;
    wxRect m_work_area;
    int m_content_height = 520;
    bool m_positioning = false;
    std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);
};

} // namespace Slic3r::GUI
