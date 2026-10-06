#include "ColorPickerDialog.hpp"

#include "GUI_App.hpp"
#include "I18N.hpp"

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/string.h>

namespace Slic3r::GUI {

ColorPickerDialog::ColorPickerDialog(wxWindow* parent, const ColorSelection& initial, ColorPickerOptions options)
    : WebViewHostDialog(parent, wxID_ANY, _L("Color Picker"), wxDefaultPosition, wxDefaultSize,
                        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_initial(initial), m_options(options)
{
    const auto normalized = normalize_color_selection(initial, options);
    if (!normalized)
        return;
    m_initial = *normalized;
    m_available = create_webview("web/dialog/ColorPickerDialog/index.html", _L("Color Picker"),
                                 wxSize(600, 660), wxSize(570, 620));
    if (!m_available)
        return;
    // The shared host attaches its sizer before constructing the full layout.
    GetSizer()->SetSizeHints(this);
    SetMinSize(FromDIP(wxSize(570, 620)));

    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { finish(wxID_CANCEL); });
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_ESCAPE)
            finish(wxID_CANCEL);
        else
            event.Skip();
    });
}

ColorPickerDialog::~ColorPickerDialog() { m_alive->store(false, std::memory_order_release); }

void ColorPickerDialog::on_script_message(const nlohmann::json& payload)
{
    // GTK/WebKit may call on the native script-message stack. All dialog work,
    // including EndModal, runs after that callback and checks the lifetime token.
    wxGetApp().CallAfter([this, alive = m_alive, payload] {
        if (alive->load(std::memory_order_acquire) && !m_closing)
            handle_web_command(payload);
    });
}

void ColorPickerDialog::handle_web_command(const nlohmann::json& payload)
{
    if (!m_available || !payload.is_object())
        return;
    const auto command = payload.find("command");
    const auto page_id = payload.find("page_id");
    if (command == payload.end() || !command->is_string() || page_id == payload.end() || !page_id->is_string())
        return;
    const auto& id = page_id->get_ref<const std::string&>();
    if (id.empty() || id.size() > 64)
        return;
    if (*command == "ready") {
        // A repeated ready from the same document must not reset its edits. A new
        // document (e.g. WebView recreation) gets a fresh initialization handshake.
        if (id != m_page_id) {
            m_page_id = id;
            m_initialized = false;
            m_init_sent = false;
        }
        if (!m_init_sent)
            send_initial_state();
        return;
    }
    if (id != m_page_id)
        return;
    if (*command == "cancel") {
        finish(wxID_CANCEL);
        return;
    }
    if (*command == "initialized") {
        if (m_init_sent)
            m_initialized = true;
        return;
    }
    if (!m_initialized)
        return;
    if (*command == "confirm") {
        const auto value = payload.find("selection");
        if (value == payload.end())
            return;
        const auto selection = color_selection_from_json(*value, m_options);
        if (!selection)
            return;
        m_selection = *selection;
        finish(wxID_OK);
    } else if (*command == "update_favorites") {
        const auto values = payload.find("favorites");
        if (values == payload.end())
            return;
        const auto favorites = color_favorites_from_json(*values);
        if (favorites)
            m_favorites = *favorites;
    }
}

void ColorPickerDialog::send_initial_state()
{
    nlohmann::json favorites = nlohmann::json::array();
    for (const ColorSelection& selection : m_favorites)
        favorites.push_back(color_selection_to_json(selection));
    const nlohmann::json payload = {{"command", "init"}, {"page_id", m_page_id},
                                   {"options", {{"allow_gradient", m_options.allow_gradient}, {"allow_alpha", m_options.allow_alpha}}},
                                   {"selection", color_selection_to_json(m_initial)}, {"favorites", std::move(favorites)}};
    m_init_sent = true;
    // Already deferred and guarded: avoid the shared call_web_handler's unguarded
    // second CallAfter, which could run after this modal dialog is destroyed.
    if (!run_script(wxString::FromUTF8("window.ColorPickerDialog.handleMessage(" + payload.dump() + ")")))
        m_init_sent = false;
}

void ColorPickerDialog::finish(int return_code)
{
    if (m_closing)
        return;
    m_closing = true;
    if (return_code != wxID_OK)
        m_selection.reset();
    if (IsModal())
        EndModal(return_code);
    else
        Hide();
}

} // namespace Slic3r::GUI
