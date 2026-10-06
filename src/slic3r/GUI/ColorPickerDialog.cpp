#include "ColorPickerDialog.hpp"
#include "ColorPickerData.hpp"
#include "Widgets/WebViewHostDialog.hpp"
#include <wx/dialog.h>
#include <wx/toplevel.h>
#include <wx/frame.h>
#include <wx/nonownedwnd.h>
#include <wx/bitmap.h>
#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/dcmemory.h>
#include <wx/display.h>
#include <wx/pen.h>
#include <wx/region.h>
#include <algorithm>
#include <cmath>
#include "ColorPickerFavorites.hpp"
#include "ColorPickerStrings.hpp"
#include <wx/webview.h>

#include "GUI_App.hpp"
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"

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
#include <wx/window.h>

namespace Slic3r::GUI {

wxPoint color_picker_panel_position(const wxRect& anchor, const wxRect& work_area, const wxSize& size, int gap)
{
    const int below = anchor.y + anchor.height + gap;
    const int above = anchor.y - gap - size.y;
    const int y = below + size.y <= work_area.y + work_area.height ? below : above;
    return {std::clamp(anchor.x, work_area.x, work_area.x + std::max(0, work_area.width - size.x)),
            std::clamp(y, work_area.y, work_area.y + std::max(0, work_area.height - size.y))};
}

ColorPickerDialog::ColorPickerDialog(wxWindow* parent, const ColorSelection& initial, ColorPickerOptions options,
                                   bool preserve_multi_color, wxWindow* anchor)
    : WebViewHostDialog(parent, wxID_ANY, _L("Color Picker"), wxDefaultPosition, wxDefaultSize,
                        wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxFRAME_SHAPED),
      m_initial(initial), m_options(options), m_preserve_multi_color(preserve_multi_color)
{
    // Snapshot the trigger instead of following the mouse or retaining a raw pointer.
    wxWindow* trigger = anchor ? anchor : parent;
    if (trigger)
        m_anchor_rect = trigger->GetScreenRect();
    int display_index = wxDisplay::GetFromPoint(m_anchor_rect.GetPosition() + wxPoint(m_anchor_rect.width / 2, m_anchor_rect.height / 2));
    if (display_index == wxNOT_FOUND)
        display_index = 0;
    m_work_area = wxDisplay(static_cast<unsigned int>(display_index)).GetClientArea();
    const auto normalized = normalize_color_selection(initial, options);
    if (!normalized)
        return;
    m_initial = *normalized;
    m_available = create_webview("web/dialog/ColorPickerDialog/index.html", _L("Color Picker"),
                                 wxSize(550, 520), wxSize(550, 300));
    if (!m_available)
        return;
    if (wxGetApp().app_config) {
        const auto stored = load_color_picker_favorites(*wxGetApp().app_config);
        m_favorites = stored.favorites;
        m_favorites_writable = stored.writable;
    }
    // The shared host attaches its sizer before constructing the full layout.
    GetSizer()->SetSizeHints(this);
    Move(m_work_area.GetPosition());
    // SetSizeHints may fit the window to the WebView's small initial best size.
    // Restore the intended dialog size only after applying the layout hints.
    position_panel();
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) { update_window_shape(); event.Skip(); });
    Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
        if (event.IsShown()) {
            position_panel();
            // GTK can require a realized window before applying its shape.
            wxGetApp().CallAfter([this, alive = m_alive] {
                if (alive->load(std::memory_order_acquire) && !m_closing)
                    update_window_shape();
            });
        }
        event.Skip();
    });
    update_window_shape();

    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { finish(wxID_CANCEL); });
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_ESCAPE)
            finish(wxID_CANCEL);
        else
            event.Skip();
    });
}

ColorPickerDialog::~ColorPickerDialog() { m_alive->store(false, std::memory_order_release); }

void ColorPickerDialog::position_panel()
{
    if (m_positioning)
        return;
    m_positioning = true;
    wxSize size = FromDIP(wxSize(550, m_content_height));
    size.x = std::min(size.x, m_work_area.width);
    size.y = std::min(size.y, std::max(1, m_work_area.height - FromDIP(16)));
    SetMinSize(wxSize(std::min(FromDIP(550), size.x), std::min(FromDIP(300), size.y)));
    if (size != GetClientSize())
        SetClientSize(size);
    const wxPoint position = color_picker_panel_position(m_anchor_rect, m_work_area, GetSize(), FromDIP(4));
    if (position != GetPosition())
        Move(position);
    m_positioning = false;
}

void ColorPickerDialog::on_dpi_changed(const wxRect&)
{
    position_panel();
    m_shape_size = wxSize();
    update_window_shape();
    Refresh();
}

void ColorPickerDialog::update_window_shape()
{
    const wxSize size = GetSize();
    if (size == m_shape_size || size.x <= 0 || size.y <= 0)
        return;
    wxBitmap bitmap(size.x, size.y, 32);
    wxMemoryDC dc(bitmap);
    dc.SetBackground(wxBrush(*wxBLACK));
    dc.Clear();
    dc.SetBrush(wxBrush(*wxWHITE));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(0, 0, size.x, size.y, FromDIP(8));
    dc.SelectObject(wxNullBitmap);
    if (SetShape(wxRegion(bitmap, *wxBLACK)))
        m_shape_size = size;
}

void ColorPickerDialog::add_user_scripts()
{
    if (wxWebView* view = browser()) {
        const std::string script = "window.ORCA_COLOR_PICKER_STRINGS = " +
            color_picker_ui_strings().dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore) + ";";
        view->AddUserScript(wxString::FromUTF8(script));
    }
}

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
    if (*command == "resize") {
        const auto height = payload.find("height");
        if (height == payload.end() || !height->is_number())
            return;
        const double value = height->get<double>();
        if (!std::isfinite(value) || value < 300 || value > 900)
            return;
        m_content_height = static_cast<int>(std::ceil(value));
        position_panel();
        return;
    }
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
        if (values == payload.end() || !m_favorites_writable || !wxGetApp().app_config)
            return;
        const auto favorites = color_favorites_from_json(*values);
        if (favorites && save_color_picker_favorites(*wxGetApp().app_config, *values)) {
            m_favorites = *favorites;
            nlohmann::json canonical = nlohmann::json::array();
            for (const ColorSelection& selection : m_favorites)
                canonical.push_back(color_selection_to_json(selection));
            const nlohmann::json response = {{"command", "favorites"}, {"page_id", m_page_id}, {"favorites", std::move(canonical)}};
            run_script(wxString::FromUTF8("window.ColorPickerDialog.handleMessage(" + response.dump() + ")"));
        }
    }
}

void ColorPickerDialog::send_initial_state()
{
    nlohmann::json favorites = nlohmann::json::array();
    for (const ColorSelection& selection : m_favorites)
        favorites.push_back(color_selection_to_json(selection));
    const nlohmann::json payload = {{"command", "init"}, {"page_id", m_page_id},
                                   {"options", {{"allow_gradient", m_options.allow_gradient}, {"allow_alpha", m_options.allow_alpha}}},
                                   {"selection", color_selection_to_json(m_initial)}, {"favorites", std::move(favorites)},
                                   {"favorites_writable", m_favorites_writable}, {"preserve_multi_color", m_preserve_multi_color}};
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
