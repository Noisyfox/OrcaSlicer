#pragma once

#include "ColorPickerData.hpp"
#include "Widgets/WebViewHostDialog.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class wxWindow;

namespace Slic3r::GUI {

class ColorPickerDialog : public WebViewHostDialog
{
public:
    ColorPickerDialog(wxWindow* parent, const ColorSelection& initial, ColorPickerOptions options = {});
    ~ColorPickerDialog() override;

    bool is_available() const { return m_available; }
    const std::optional<ColorSelection>& selection() const { return m_selection; }

private:
    void on_script_message(const nlohmann::json& payload) override;
    void handle_web_command(const nlohmann::json& payload);
    void send_initial_state();
    void finish(int return_code);

    ColorSelection m_initial;
    ColorPickerOptions m_options;
    std::optional<ColorSelection> m_selection;
    std::vector<ColorSelection> m_favorites;
    std::string m_page_id;
    bool m_available = false;
    bool m_init_sent = false;
    bool m_initialized = false;
    bool m_closing = false;
    bool m_favorites_writable = false;
    std::shared_ptr<std::atomic<bool>> m_alive = std::make_shared<std::atomic<bool>>(true);
};

} // namespace Slic3r::GUI
