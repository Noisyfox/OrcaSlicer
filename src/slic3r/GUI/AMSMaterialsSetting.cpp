#include "AMSMaterialsSetting.hpp"
#include "ExtrusionCalibration.hpp"
#include "MsgDialog.hpp"
#include "GUI_App.hpp"
#include "libslic3r/Preset.hpp"
#include "I18N.hpp"
#include <wx/dcgraph.h>

namespace Slic3r { namespace GUI {

wxDEFINE_EVENT(EVT_SELECTED_COLOR, wxCommandEvent);

AMSMaterialsSetting::AMSMaterialsSetting(wxWindow *parent, wxWindowID id) 
    : DPIDialog(parent, id, _L("AMS Materials Setting"), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_color_picker_popup(ColorPickerPopup(this))
{
    create();
    wxGetApp().UpdateDlgDarkUI(this);
}

void AMSMaterialsSetting::create()
{
    SetBackgroundColour(*wxWHITE);
    wxBoxSizer *m_sizer_main = new wxBoxSizer(wxVERTICAL);

    m_panel_normal = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    create_panel_normal(m_panel_normal);
    m_panel_kn = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    create_panel_kn(m_panel_kn);

    wxBoxSizer *m_sizer_button = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_button->Add(0, 0, 1, wxEXPAND, 0);

    m_button_confirm = new Button(this, _L("Confirm"));
    m_btn_bg_green   = StateColor(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed), std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
                            std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal));
    m_button_confirm->SetBackgroundColor(m_btn_bg_green);
    m_button_confirm->SetBorderColor(wxColour(0, 150, 136));
    m_button_confirm->SetTextColor(wxColour("#FFFFFE"));
    m_button_confirm->SetMinSize(AMS_MATERIALS_SETTING_BUTTON_SIZE);
    m_button_confirm->SetCornerRadius(FromDIP(12));
    m_button_confirm->Bind(wxEVT_BUTTON, &AMSMaterialsSetting::on_select_ok, this);

    m_button_reset = new Button(this, _L("Reset"));
    m_btn_bg_gray = StateColor(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed), std::pair<wxColour, int>(*wxWHITE, StateColor::Focused),
        std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));
    m_button_reset->SetBackgroundColor(m_btn_bg_gray);
    m_button_reset->SetBorderColor(AMS_MATERIALS_SETTING_GREY900);
    m_button_reset->SetTextColor(AMS_MATERIALS_SETTING_GREY900);
    m_button_reset->SetMinSize(AMS_MATERIALS_SETTING_BUTTON_SIZE);
    m_button_reset->SetCornerRadius(FromDIP(12));
    m_button_reset->Bind(wxEVT_BUTTON, &AMSMaterialsSetting::on_select_reset, this);

    m_button_close = new Button(this, _L("Close"));
    m_button_close->SetBackgroundColor(m_btn_bg_gray);
    m_button_close->SetBorderColor(AMS_MATERIALS_SETTING_GREY900);
    m_button_close->SetTextColor(AMS_MATERIALS_SETTING_GREY900);
    m_button_close->SetMinSize(AMS_MATERIALS_SETTING_BUTTON_SIZE);
    m_button_close->SetCornerRadius(FromDIP(12));
    m_button_close->Bind(wxEVT_BUTTON, &AMSMaterialsSetting::on_select_close, this);

    m_sizer_button->Add(m_button_confirm, 0, wxALIGN_CENTER | wxRIGHT, FromDIP(20));
    m_sizer_button->Add(m_button_reset, 0, wxALIGN_CENTER | wxRIGHT, FromDIP(20));
    m_sizer_button->Add(m_button_close, 0, wxALIGN_CENTER, 0);

    m_sizer_main->Add(m_panel_normal, 0, wxALL, FromDIP(2));
    
    m_sizer_main->Add(m_panel_kn, 0, wxALL, FromDIP(2));

    m_sizer_main->Add(0, 0, 0, wxTOP, FromDIP(24));
    m_sizer_main->Add(m_sizer_button, 0,  wxEXPAND | wxLEFT | wxRIGHT, FromDIP(20));
    m_sizer_main->Add(0, 0, 0,  wxTOP, FromDIP(16));

    SetSizer(m_sizer_main);
    Layout();
    Fit();

    m_input_nozzle_min->GetTextCtrl()->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) {
        warning_text->Hide();
        Layout();
        Fit();
        e.Skip();
        });
    m_input_nozzle_min->GetTextCtrl()->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) {
        input_min_finish();
        e.Skip();
        });
    m_input_nozzle_min->GetTextCtrl()->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
        input_min_finish();
        e.Skip();
        });

    m_input_nozzle_max->GetTextCtrl()->Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) {
        warning_text->Hide();
        Layout();
        Fit();
        e.Skip();
        });
    m_input_nozzle_max->GetTextCtrl()->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) {
        input_max_finish();
        e.Skip();
        });
    m_input_nozzle_max->GetTextCtrl()->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
        input_max_finish();
        e.Skip();
        });

    Bind(wxEVT_PAINT, &AMSMaterialsSetting::paintEvent, this);
    Bind(EVT_SELECTED_COLOR, &AMSMaterialsSetting::on_picker_color, this);
     m_comboBox_filament->Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(AMSMaterialsSetting::on_select_filament), NULL, this);
}

void AMSMaterialsSetting::create_panel_normal(wxWindow* parent)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* m_sizer_filament = new wxBoxSizer(wxHORIZONTAL);

    m_title_filament = new wxStaticText(parent, wxID_ANY, _L("Filament"), wxDefaultPosition, wxSize(AMS_MATERIALS_SETTING_LABEL_WIDTH, -1), 0);
    m_title_filament->SetFont(::Label::Body_13);
    m_title_filament->SetForegroundColour(AMS_MATERIALS_SETTING_GREY800);
    m_title_filament->Wrap(-1);
    m_sizer_filament->Add(m_title_filament, 0, wxALIGN_CENTER, 0);

    m_sizer_filament->Add(0, 0, 0, wxEXPAND, 0);

#ifdef __APPLE__
    m_comboBox_filament = new wxComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, AMS_MATERIALS_SETTING_COMBOX_WIDTH, 0, nullptr, wxCB_READONLY);
#else
    m_comboBox_filament = new ::ComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, AMS_MATERIALS_SETTING_COMBOX_WIDTH, 0, nullptr, wxCB_READONLY);
#endif

    m_sizer_filament->Add(m_comboBox_filament, 1, wxALIGN_CENTER, 0);

    m_readonly_filament = new TextInput(parent, wxEmptyString, "", "", wxDefaultPosition, AMS_MATERIALS_SETTING_COMBOX_WIDTH, wxTE_READONLY | wxRIGHT);
    m_readonly_filament->SetBorderColor(StateColor(std::make_pair(0xDBDBDB, (int)StateColor::Focused), std::make_pair(0x009688, (int)StateColor::Hovered),
        std::make_pair(0xDBDBDB, (int)StateColor::Normal)));
    m_readonly_filament->SetFont(::Label::Body_14);
    m_readonly_filament->SetLabelColor(AMS_MATERIALS_SETTING_GREY800);
    m_readonly_filament->GetTextCtrl()->Bind(wxEVT_SET_FOCUS, [](auto& e) {});
    m_readonly_filament->GetTextCtrl()->Hide();
    m_sizer_filament->Add(m_readonly_filament, 1, wxALIGN_CENTER, 0);
    m_readonly_filament->Hide();

    wxBoxSizer* m_sizer_colour = new wxBoxSizer(wxHORIZONTAL);

    m_title_colour = new wxStaticText(parent, wxID_ANY, _L("Colour"), wxDefaultPosition, wxSize(AMS_MATERIALS_SETTING_LABEL_WIDTH, -1), 0);
    m_title_colour->SetFont(::Label::Body_13);
    m_title_colour->SetForegroundColour(AMS_MATERIALS_SETTING_GREY800);
    m_title_colour->Wrap(-1);
    m_sizer_colour->Add(m_title_colour, 0, wxALIGN_CENTER, 0);

    m_sizer_colour->Add(0, 0, 0, wxEXPAND, 0);

    m_clr_picker = new ColorPicker(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_clr_picker->set_show_full(true);
    m_clr_picker->SetBackgroundColour(*wxWHITE);


    m_clr_picker->Bind(wxEVT_LEFT_DOWN, &AMSMaterialsSetting::on_clr_picker, this);
    m_sizer_colour->Add(m_clr_picker, 0, 0, 0);

    wxBoxSizer* m_sizer_temperature = new wxBoxSizer(wxHORIZONTAL);
    m_title_temperature = new wxStaticText(parent, wxID_ANY, _L("Nozzle\nTemperature"), wxDefaultPosition, wxSize(AMS_MATERIALS_SETTING_LABEL_WIDTH, -1), 0);
    m_title_temperature->SetFont(::Label::Body_13);
    m_title_temperature->SetForegroundColour(AMS_MATERIALS_SETTING_GREY800);
    m_title_temperature->Wrap(-1);
    m_sizer_temperature->Add(m_title_temperature, 0, wxALIGN_CENTER, 0);

    m_sizer_temperature->Add(0, 0, 0, wxEXPAND, 0);

    wxBoxSizer* sizer_other = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_tempinput = new wxBoxSizer(wxHORIZONTAL);

    m_input_nozzle_max = new ::TextInput(parent, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, AMS_MATERIALS_SETTING_INPUT_SIZE, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    m_input_nozzle_min = new ::TextInput(parent, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, AMS_MATERIALS_SETTING_INPUT_SIZE, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    m_input_nozzle_max->Enable(false);
    m_input_nozzle_min->Enable(false);

    m_input_nozzle_max->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_input_nozzle_max->GetTextCtrl()->SetSize(wxSize(-1, FromDIP(20)));
    m_input_nozzle_min->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    m_input_nozzle_min->GetTextCtrl()->SetSize(wxSize(-1, FromDIP(20)));

    auto bitmap_max_degree = new wxStaticBitmap(parent, -1, create_scaled_bitmap("degree", nullptr, 16), wxDefaultPosition, wxDefaultSize);
    auto bitmap_min_degree = new wxStaticBitmap(parent, -1, create_scaled_bitmap("degree", nullptr, 16), wxDefaultPosition, wxDefaultSize);

    sizer_tempinput->Add(m_input_nozzle_max, 1, wxALIGN_CENTER, 0);
    sizer_tempinput->Add(bitmap_min_degree, 0, wxALIGN_CENTER, 0);
    sizer_tempinput->Add(FromDIP(10), 0, 0, 0);
    sizer_tempinput->Add(m_input_nozzle_min, 1, wxALIGN_CENTER, 0);
    sizer_tempinput->Add(bitmap_max_degree, 0, wxALIGN_CENTER, 0);

    wxBoxSizer* sizer_temp_txt = new wxBoxSizer(wxHORIZONTAL);
    auto        m_title_max = new wxStaticText(parent, wxID_ANY, _L("max"), wxDefaultPosition, AMS_MATERIALS_SETTING_INPUT_SIZE);
    m_title_max->SetForegroundColour(AMS_MATERIALS_SETTING_GREY800);
    m_title_max->SetFont(::Label::Body_13);
    auto m_title_min = new wxStaticText(parent, wxID_ANY, _L("min"), wxDefaultPosition, AMS_MATERIALS_SETTING_INPUT_SIZE);
    m_title_min->SetForegroundColour(AMS_MATERIALS_SETTING_GREY800);
    m_title_min->SetFont(::Label::Body_13);
    sizer_temp_txt->Add(m_title_max, 1, wxALIGN_CENTER, 0);
    sizer_temp_txt->Add(FromDIP(10), 0, 0, 0);
    sizer_temp_txt->Add(m_title_min, 1, wxALIGN_CENTER | wxRIGHT, FromDIP(16));


    sizer_other->Add(sizer_temp_txt, 0, wxALIGN_CENTER, 0);
    sizer_other->Add(sizer_tempinput, 0, wxALIGN_CENTER, 0);

    m_sizer_temperature->Add(sizer_other, 0, wxALL | wxALIGN_CENTER, 0);
    m_sizer_temperature->AddStretchSpacer();

    wxString warning_string = wxString::FromUTF8(
        (boost::format(_u8L("The input value should be greater than %1% and less than %2%")) % FILAMENT_MIN_TEMP % FILAMENT_MAX_TEMP).str());
    warning_text = new wxStaticText(parent, wxID_ANY, warning_string, wxDefaultPosition, wxDefaultSize, 0);
    warning_text->SetFont(::Label::Body_13);
    warning_text->SetForegroundColour(wxColour(255, 111, 0));

    warning_text->Wrap(AMS_MATERIALS_SETTING_BODY_WIDTH);
    warning_text->SetMinSize(wxSize(AMS_MATERIALS_SETTING_BODY_WIDTH, -1));
    warning_text->Hide();

    m_panel_SN = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer* m_sizer_SN = new wxBoxSizer(wxVERTICAL);
    m_sizer_SN->AddSpacer(FromDIP(16));

    wxBoxSizer* m_sizer_SN_inside = new wxBoxSizer(wxHORIZONTAL);

    auto m_title_SN = new wxStaticText(m_panel_SN, wxID_ANY, _L("SN"), wxDefaultPosition, wxSize(AMS_MATERIALS_SETTING_LABEL_WIDTH, -1), 0);
    m_title_SN->SetFont(::Label::Body_13);
    m_title_SN->SetForegroundColour(AMS_MATERIALS_SETTING_GREY800);
    m_title_SN->Wrap(-1);
    m_sizer_SN_inside->Add(m_title_SN, 0, wxALIGN_CENTER, 0);

    m_sizer_SN_inside->Add(0, 0, 0, wxEXPAND, 0);

    m_sn_number = new wxStaticText(m_panel_SN, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    m_sn_number->SetForegroundColour(*wxBLACK);
    m_sizer_SN_inside->Add(m_sn_number, 0, wxALIGN_CENTER, 0);
    m_sizer_SN->Add(m_sizer_SN_inside);

    m_panel_SN->SetSizer(m_sizer_SN);
    m_panel_SN->Layout();
    m_panel_SN->Fit();

    wxBoxSizer* m_tip_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_tip_readonly = new wxStaticText(parent, wxID_ANY, _L("Setting AMS slot information while printing is not supported"), wxDefaultPosition, wxSize(-1, AMS_MATERIALS_SETTING_INPUT_SIZE.y));
    m_tip_readonly->SetForegroundColour(*wxBLACK);
    m_tip_readonly->Hide();
    m_tip_sizer->Add(m_tip_readonly, 0, wxALIGN_CENTER | wxRIGHT, FromDIP(20));

    sizer->Add(0, 0, 0, wxTOP, FromDIP(16));
    sizer->Add(m_sizer_filament, 0, wxLEFT | wxRIGHT, FromDIP(20));
    sizer->Add(0, 0, 0, wxTOP, FromDIP(16));
    sizer->Add(m_sizer_colour, 0, wxLEFT | wxRIGHT, FromDIP(20));
    sizer->Add(0, 0, 0, wxTOP, FromDIP(16));
    sizer->Add(m_sizer_temperature, 0, wxLEFT | wxRIGHT, FromDIP(20));
    sizer->Add(0, 0, 0, wxTOP, FromDIP(5));
    sizer->Add(warning_text, 0, wxLEFT | wxRIGHT, FromDIP(20));
    sizer->Add(m_panel_SN, 0, wxLEFT, FromDIP(20));
    sizer->Add(0, 0, 0, wxTOP, FromDIP(24));
    sizer->Add(m_tip_sizer, 0, wxLEFT, FromDIP(20));
    parent->SetSizer(sizer);
}

void AMSMaterialsSetting::create_panel_kn(wxWindow* parent)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    // title
    m_ratio_text = new wxStaticText(parent, wxID_ANY, _L("Factors of dynamic flow cali"));
    m_ratio_text->SetForegroundColour(wxColour(50, 58, 61));
    m_ratio_text->SetFont(Label::Head_14);

    auto kn_val_sizer = new wxFlexGridSizer(0, 2, 0, 0);
    kn_val_sizer->SetFlexibleDirection(wxBOTH);
    kn_val_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
    kn_val_sizer->AddGrowableCol(1);

    // k params input
    m_k_param = new wxStaticText(parent, wxID_ANY, _L("Factor K"), wxDefaultPosition, wxDefaultSize, 0);
    m_k_param->SetFont(::Label::Body_13);
    m_k_param->SetForegroundColour(wxColour(50, 58, 61));
    m_k_param->Wrap(-1);
    kn_val_sizer->Add(m_k_param, 0, wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    m_input_k_val = new TextInput(parent, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    m_input_k_val->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    kn_val_sizer->Add(m_input_k_val, 0, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    // n params input
    wxBoxSizer* n_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_n_param = new wxStaticText(parent, wxID_ANY, _L("Factor N"), wxDefaultPosition, wxDefaultSize, 0);
    m_n_param->SetFont(::Label::Body_13);
    m_n_param->SetForegroundColour(wxColour(50, 58, 61));
    m_n_param->Wrap(-1);
    kn_val_sizer->Add(m_n_param, 1, wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));
    m_input_n_val = new TextInput(parent, wxEmptyString, wxEmptyString, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CENTRE | wxTE_PROCESS_ENTER);
    m_input_n_val->GetTextCtrl()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    kn_val_sizer->Add(m_input_n_val, 0, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    // hide n
    m_n_param->Hide();
    m_input_n_val->Hide();

    sizer->Add(0, 0, 0, wxTOP, FromDIP(10));
    sizer->Add(m_ratio_text, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(20));
    sizer->Add(0, 0, 0, wxTOP, FromDIP(10));
    sizer->Add(kn_val_sizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(20));
    sizer->Add(0, 0, 0, wxTOP, FromDIP(10));
    parent->SetSizer(sizer);
}

void AMSMaterialsSetting::paintEvent(wxPaintEvent &evt) 
{
    auto      size = GetSize();
    wxPaintDC dc(this);
    dc.SetPen(wxPen(StateColor::darkModeColorFor(wxColour("#000000")), 1, wxSOLID));
    dc.SetBrush(wxBrush(*wxTRANSPARENT_BRUSH));
    dc.DrawRectangle(0, 0, size.x, size.y);
}

AMSMaterialsSetting::~AMSMaterialsSetting()
{
    m_comboBox_filament->Disconnect(wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler(AMSMaterialsSetting::on_select_filament), NULL, this);
}

void AMSMaterialsSetting::input_min_finish() 
{
    if (m_input_nozzle_min->GetTextCtrl()->GetValue().empty()) return;

    auto val = std::atoi(m_input_nozzle_min->GetTextCtrl()->GetValue().c_str());

    if (val < FILAMENT_MIN_TEMP || val > FILAMENT_MAX_TEMP) {
        warning_text->Show();
    } else {
        warning_text->Hide();
    }
    Layout();
    Fit();
}

void AMSMaterialsSetting::input_max_finish()
{
    if (m_input_nozzle_max->GetTextCtrl()->GetValue().empty()) return;

    auto val = std::atoi(m_input_nozzle_max->GetTextCtrl()->GetValue().c_str());

    if (val < FILAMENT_MIN_TEMP || val > FILAMENT_MAX_TEMP) {
        warning_text->Show();
    }
    else {
        warning_text->Hide();
    }
    Layout();
    Fit();
}

void AMSMaterialsSetting::update()
{
    if (obj) {
        update_widgets();
        if (obj->is_in_printing() || obj->can_resume()) {
            enable_confirm_button(false);
        } else {
            enable_confirm_button(true);
        }
    }
}

void AMSMaterialsSetting::enable_confirm_button(bool en)
{
    m_button_confirm->Show(en);
    if (!m_is_third) {
        m_tip_readonly->Hide(); 
    }
    else {
        m_comboBox_filament->Show(en);
        m_readonly_filament->Show(!en);

        if ( !is_virtual_tray() ) {
            m_tip_readonly->SetLabelText(_L("Setting AMS slot information while printing is not supported"));
        }
        else {
            m_tip_readonly->SetLabelText(_L("Setting Virtual slot information while printing is not supported"));
        }
        m_tip_readonly->Show(!en);
    }
}

void AMSMaterialsSetting::on_select_reset(wxCommandEvent& event) {
    MessageDialog msg_dlg(nullptr, _L("Are you sure you want to clear the filament information?"), wxEmptyString, wxICON_WARNING | wxOK | wxCANCEL);
    auto result = msg_dlg.ShowModal();
    if (result != wxID_OK)
        return;

    m_input_nozzle_min->GetTextCtrl()->SetValue("");
    m_input_nozzle_max->GetTextCtrl()->SetValue("");
    ams_filament_id = "";
    ams_setting_id = "";
    wxString k_text = "0.000";
    wxString n_text = "0.000";
    m_filament_type = "";
    long nozzle_temp_min_int = 0;
    long nozzle_temp_max_int = 0;
    wxColour color = *wxWHITE;
    char col_buf[10];
    sprintf(col_buf, "%02X%02X%02XFF", (int)color.Red(), (int)color.Green(), (int)color.Blue());

    if (obj) {
        // set filament
        if (obj->is_support_filament_edit_virtual_tray || !is_virtual_tray()) {
            if (is_virtual_tray()) {
                obj->command_ams_filament_settings(255, VIRTUAL_TRAY_ID, ams_filament_id, ams_setting_id, std::string(col_buf), m_filament_type, nozzle_temp_min_int, nozzle_temp_max_int);
            }
            else {
                obj->command_ams_filament_settings(ams_id, tray_id, ams_filament_id, ams_setting_id, std::string(col_buf), m_filament_type, nozzle_temp_min_int, nozzle_temp_max_int);
            }
        }

        // set k / n value
        if (obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
            // set extrusion cali ratio
            int cali_tray_id = ams_id * 4 + tray_id;

            double k = 0.0;
            try {
                k_text.ToDouble(&k);
            }
            catch (...) {
                ;
            }

            double n = 0.0;
            try {
                n_text.ToDouble(&n);
            }
            catch (...) {
                ;
            }
            obj->command_extrusion_cali_set(cali_tray_id, "", "", k, n);
        }
    }
    Close();
}

void AMSMaterialsSetting::on_select_ok(wxCommandEvent &event)
{
    wxString k_text = m_input_k_val->GetTextCtrl()->GetValue();
    wxString n_text = m_input_n_val->GetTextCtrl()->GetValue();

    if (is_virtual_tray() && obj && !obj->is_support_filament_edit_virtual_tray) {
        if (!ExtrusionCalibration::check_k_validation(k_text)) {
            wxString k_tips = _L("Please input a valid value (K in 0~0.5)");
            wxString kn_tips = _L("Please input a valid value (K in 0~0.5, N in 0.6~2.0)");
            MessageDialog msg_dlg(nullptr, k_tips, wxEmptyString, wxICON_WARNING | wxOK);
            msg_dlg.ShowModal();
            return;
        }
        double k = 0.0;
        try {
            k_text.ToDouble(&k);
        }
        catch (...) {
            ;
        }
        double n = 0.0;
        try {
            n_text.ToDouble(&n);
        }
        catch (...) {
            ;
        }
        obj->command_extrusion_cali_set(VIRTUAL_TRAY_ID, "", "", k, n);
        Close();
    }
    else {
        if (!m_is_third) {
            // check and set k n
            if (obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
                if (!ExtrusionCalibration::check_k_validation(k_text)) {
                    wxString k_tips = _L("Please input a valid value (K in 0~0.5)");
                    wxString kn_tips = _L("Please input a valid value (K in 0~0.5, N in 0.6~2.0)");
                    MessageDialog msg_dlg(nullptr, k_tips, wxEmptyString, wxICON_WARNING | wxOK);
                    msg_dlg.ShowModal();
                    return;
                }
            }


            // set k / n value
            if (obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
                // set extrusion cali ratio
                int cali_tray_id = ams_id * 4 + tray_id;

                double k = 0.0;
                try {
                    k_text.ToDouble(&k);
                }
                catch (...) {
                    ;
                }

                double n = 0.0;
                try {
                    n_text.ToDouble(&n);
                }
                catch (...) {
                    ;
                }
                obj->command_extrusion_cali_set(cali_tray_id, "", "", k, n);
            }
            Close();
            return;
        }
        wxString nozzle_temp_min = m_input_nozzle_min->GetTextCtrl()->GetValue();
        auto     filament = m_comboBox_filament->GetValue();

        wxString nozzle_temp_max = m_input_nozzle_max->GetTextCtrl()->GetValue();

        long nozzle_temp_min_int, nozzle_temp_max_int;
        nozzle_temp_min.ToLong(&nozzle_temp_min_int);
        nozzle_temp_max.ToLong(&nozzle_temp_max_int);
        wxColour color = m_clr_picker->m_colour;
        char col_buf[10];
        sprintf(col_buf, "%02X%02X%02XFF", (int)color.Red(), (int)color.Green(), (int)color.Blue());
        ams_filament_id = "";
        ams_setting_id = "";

        PresetBundle* preset_bundle = wxGetApp().preset_bundle;
        if (preset_bundle) {
            for (auto it = preset_bundle->filaments.begin(); it != preset_bundle->filaments.end(); it++) {

                if (it->alias.compare(m_comboBox_filament->GetValue().ToStdString()) == 0) {


                    //check is it in the filament blacklist
                    if(!is_virtual_tray()){
                        bool in_blacklist = false;
                        std::string action;
                        std::string info;
                        std::string filamnt_type;
                        it->get_filament_type(filamnt_type);

                        if (it->vendor) {
                            DeviceManager::check_filaments_in_blacklist(it->vendor->name, filamnt_type, in_blacklist, action, info);
                        }

                        if (in_blacklist) {
                            if (action == "prohibition") {
                                MessageDialog msg_wingow(nullptr, info, _L("Error"), wxICON_WARNING | wxOK);
                                msg_wingow.ShowModal();
                                //m_comboBox_filament->SetSelection(m_filament_selection);
                                return;
                            }
                            else if (action == "warning") {
                                MessageDialog msg_wingow(nullptr, info, _L("Warning"), wxICON_INFORMATION | wxOK);
                                msg_wingow.ShowModal();
                            }
                        }
                    }

                    ams_filament_id = it->filament_id;
                    ams_setting_id = it->setting_id;
                    break;
                }
            }
        }

        if (ams_filament_id.empty() || nozzle_temp_min.empty() || nozzle_temp_max.empty() || m_filament_type.empty()) {
            BOOST_LOG_TRIVIAL(trace) << "Invalid Setting id";
            MessageDialog msg_dlg(nullptr, _L("You need to select the material type and color first."), wxEmptyString, wxICON_WARNING | wxOK);
            msg_dlg.ShowModal();
            return;
        }
        else {
            if (obj) {
                if (obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
                    if (!ExtrusionCalibration::check_k_validation(k_text)) {
                        wxString k_tips = _L("Please input a valid value (K in 0~0.5)");
                        wxString kn_tips = _L("Please input a valid value (K in 0~0.5, N in 0.6~2.0)");
                        MessageDialog msg_dlg(nullptr, k_tips, wxEmptyString, wxICON_WARNING | wxOK);
                        msg_dlg.ShowModal();
                        return;
                    }
                }

                // set filament
                if (obj->is_support_filament_edit_virtual_tray || !is_virtual_tray()) {
                    if (is_virtual_tray()) {
                        obj->command_ams_filament_settings(255, VIRTUAL_TRAY_ID, ams_filament_id, ams_setting_id, std::string(col_buf), m_filament_type, nozzle_temp_min_int, nozzle_temp_max_int);
                    }
                    else {
                        obj->command_ams_filament_settings(ams_id, tray_id, ams_filament_id, ams_setting_id, std::string(col_buf), m_filament_type, nozzle_temp_min_int, nozzle_temp_max_int);
                    }
                }

                // set k / n value
                if (obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
                    // set extrusion cali ratio
                    int cali_tray_id = ams_id * 4 + tray_id;

                    double k = 0.0;
                    try {
                        k_text.ToDouble(&k);
                    }
                    catch (...) {
                        ;
                    }

                    double n = 0.0;
                    try {
                        n_text.ToDouble(&n);
                    }
                    catch (...) {
                        ;
                    }
                    obj->command_extrusion_cali_set(cali_tray_id, "", "", k, n);
                }
            }
        }
    }
    Close();
}

void AMSMaterialsSetting::on_select_close(wxCommandEvent &event)
{
    Close();
}

void AMSMaterialsSetting::set_color(wxColour color)
{
    //m_clrData->SetColour(color);
    m_clr_picker->set_color(color);
}

void AMSMaterialsSetting::set_colors(std::vector<wxColour> colors)
{
    //m_clrData->SetColour(color);
    m_clr_picker->set_colors(colors);
}


void AMSMaterialsSetting::on_picker_color(wxCommandEvent& event)
{
    unsigned int color_num  = event.GetInt();
    set_color(wxColour(color_num>>16&0xFF, color_num>>8&0xFF, color_num&0xFF));
}

void AMSMaterialsSetting::on_clr_picker(wxMouseEvent &event) 
{
    if(!m_is_third || obj->is_in_printing() || obj->can_resume())
        return;


    std::vector<wxColour> ams_colors;
    for (auto ams_it = obj->amsList.begin(); ams_it != obj->amsList.end(); ++ams_it) {
        for (auto tray_id = ams_it->second->trayList.begin(); tray_id != ams_it->second->trayList.end(); ++tray_id) {
            std::vector<wxColour>::iterator iter = find(ams_colors.begin(), ams_colors.end(), AmsTray::decode_color(tray_id->second->color));
            if (iter == ams_colors.end()) {
                ams_colors.push_back(AmsTray::decode_color(tray_id->second->color));
            }
        }
    }

    wxPoint img_pos = m_clr_picker->ClientToScreen(wxPoint(0, 0));
    wxPoint popup_pos(img_pos.x + FromDIP(50), img_pos.y);
    m_color_picker_popup.Position(popup_pos, wxSize(0, 0));
    m_color_picker_popup.set_ams_colours(ams_colors);
    m_color_picker_popup.set_def_colour(m_clr_picker->m_colour);
    m_color_picker_popup.Popup();

    /*auto clr_dialog = new wxColourDialog(this, m_clrData);
    if (clr_dialog->ShowModal() == wxID_OK) {
        m_clrData = &(clr_dialog->GetColourData());
        m_clr_picker->SetBackgroundColor(wxColour(
            m_clrData->GetColour().Red(),
            m_clrData->GetColour().Green(),
            m_clrData->GetColour().Blue(),
            254
        ));
    }*/
}

bool AMSMaterialsSetting::is_virtual_tray()
{
    if (tray_id == VIRTUAL_TRAY_ID)
        return true;
    return false;
}

void AMSMaterialsSetting::update_widgets()
{
    // virtual tray
    if (is_virtual_tray()) {
        if (obj && obj->is_support_filament_edit_virtual_tray)
            m_panel_normal->Show();
        else
            m_panel_normal->Hide();
        m_panel_kn->Show();
    } else if (obj && obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
        m_panel_normal->Show();
        m_panel_kn->Show();
    } else {
        m_panel_normal->Show();
        m_panel_kn->Hide();
    }
    Layout();
}

bool AMSMaterialsSetting::Show(bool show) 
{ 
    if (show) {
        m_button_confirm->SetMinSize(AMS_MATERIALS_SETTING_BUTTON_SIZE);
        m_input_nozzle_max->GetTextCtrl()->SetSize(wxSize(-1, FromDIP(20)));
        m_input_nozzle_min->GetTextCtrl()->SetSize(wxSize(-1, FromDIP(20)));
        //m_clr_picker->set_color(m_clr_picker->GetParent()->GetBackgroundColour());

        if (obj && obj->is_function_supported(PrinterFunction::FUNC_EXTRUSION_CALI)) {
            m_ratio_text->Show();
            m_k_param->Show();
            m_input_k_val->Show();
        }
        else {
            m_ratio_text->Hide();
            m_k_param->Hide();
            m_input_k_val->Hide();
        }
        Layout();
        Fit();
        wxGetApp().UpdateDarkUI(this);
    }
    return DPIDialog::Show(show); 
}

void AMSMaterialsSetting::Popup(wxString filament, wxString sn, wxString temp_min, wxString temp_max, wxString k, wxString n)
{
    update_widgets();
    // set default value
    if (k.IsEmpty())
        k = "0.000";
    if (n.IsEmpty())
        n = "0.000";

    m_input_k_val->GetTextCtrl()->SetValue(k);
    m_input_n_val->GetTextCtrl()->SetValue(n);

    if (is_virtual_tray() && obj && !obj->is_support_filament_edit_virtual_tray) {
        m_button_reset->Show();
        m_button_confirm->Show();
        update();
        Layout();
        Fit();
        ShowModal();
        return;
    } else {
       /* m_clr_picker->set_color(wxColour(
            m_clrData->GetColour().Red(),
            m_clrData->GetColour().Green(),
            m_clrData->GetColour().Blue(),
            254
        ));*/

        if (!m_is_third) {
            m_button_reset->Hide();
            if (obj && obj->is_function_supported(PrinterFunction::FUNC_VIRTUAL_TYAY)) {
                m_button_confirm->Show();
            } else {
                m_button_confirm->Hide();
            }

            m_sn_number->SetLabel(sn);
            m_panel_SN->Show();
            m_comboBox_filament->Hide();
            m_readonly_filament->Show();
            //m_readonly_filament->GetTextCtrl()->SetLabel("Bambu " + filament);
            m_readonly_filament->SetLabel("Bambu " + filament);
            m_input_nozzle_min->GetTextCtrl()->SetValue(temp_min);
            m_input_nozzle_max->GetTextCtrl()->SetValue(temp_max);

            update();
            Layout();
            Fit();
            ShowModal();
            return;
        }

        m_button_reset->Show();
        m_button_confirm->Show();
        m_panel_SN->Hide();
        m_comboBox_filament->Show();
        m_readonly_filament->Hide();


        int selection_idx = -1, idx = 0;
        wxArrayString filament_items;
        std::set<std::string> filament_id_set;

        PresetBundle* preset_bundle = wxGetApp().preset_bundle;
        if (preset_bundle) {
            BOOST_LOG_TRIVIAL(trace) << "system_preset_bundle filament number=" << preset_bundle->filaments.size();
            for (auto filament_it = preset_bundle->filaments.begin(); filament_it != preset_bundle->filaments.end(); filament_it++) {
                // filter by system preset
                if (!filament_it->is_system) continue;

                for (auto printer_it = preset_bundle->printers.begin(); printer_it != preset_bundle->printers.end(); printer_it++) {
                    // filter by system preset
                    if (!printer_it->is_system) continue;
                    // get printer_model
                    ConfigOption* printer_model_opt = printer_it->config.option("printer_model");
                    ConfigOptionString* printer_model_str = dynamic_cast<ConfigOptionString*>(printer_model_opt);
                    if (!printer_model_str || !obj)
                        continue;

                    // use printer_model as printer type
                    if (printer_model_str->value != MachineObject::get_preset_printer_model_name(obj->printer_type))
                        continue;
                    ConfigOption* printer_opt = filament_it->config.option("compatible_printers");
                    ConfigOptionStrings* printer_strs = dynamic_cast<ConfigOptionStrings*>(printer_opt);
                    for (auto printer_str : printer_strs->values) {
                        if (printer_it->name == printer_str) {
                            if (filament_id_set.find(filament_it->filament_id) != filament_id_set.end()) {
                                continue;
                            }
                            else {
                                filament_id_set.insert(filament_it->filament_id);
                                // name matched
                                filament_items.push_back(filament_it->alias);
                                if (filament_it->filament_id == ams_filament_id) {
                                    selection_idx = idx;

                                    // update if nozzle_temperature_range is found
                                    ConfigOption* opt_min = filament_it->config.option("nozzle_temperature_range_low");
                                    if (opt_min) {
                                        ConfigOptionInts* opt_min_ints = dynamic_cast<ConfigOptionInts*>(opt_min);
                                        if (opt_min_ints) {
                                            wxString text_nozzle_temp_min = wxString::Format("%d", opt_min_ints->get_at(0));
                                            m_input_nozzle_min->GetTextCtrl()->SetValue(text_nozzle_temp_min);
                                        }
                                    }
                                    ConfigOption* opt_max = filament_it->config.option("nozzle_temperature_range_high");
                                    if (opt_max) {
                                        ConfigOptionInts* opt_max_ints = dynamic_cast<ConfigOptionInts*>(opt_max);
                                        if (opt_max_ints) {
                                            wxString text_nozzle_temp_max = wxString::Format("%d", opt_max_ints->get_at(0));
                                            m_input_nozzle_max->GetTextCtrl()->SetValue(text_nozzle_temp_max);
                                        }
                                    }
                                }
                                idx++;
                            }
                        }
                    }
                }
            }
            m_comboBox_filament->Set(filament_items);
            m_comboBox_filament->SetSelection(selection_idx);
            post_select_event();
        }
    }

    update();
    Layout();
    Fit();
    ShowModal();
}

void AMSMaterialsSetting::post_select_event() {
    wxCommandEvent event(wxEVT_COMBOBOX);
    event.SetEventObject(m_comboBox_filament);
    wxPostEvent(m_comboBox_filament, event);
}

void AMSMaterialsSetting::msw_rescale()
{
    m_clr_picker->msw_rescale();
}

void AMSMaterialsSetting::on_select_filament(wxCommandEvent &evt)
{
    m_filament_type = "";
    PresetBundle* preset_bundle = wxGetApp().preset_bundle;
    if (preset_bundle) {
        for (auto it = preset_bundle->filaments.begin(); it != preset_bundle->filaments.end(); it++) {
            if (!m_comboBox_filament->GetValue().IsEmpty() && it->alias.compare(m_comboBox_filament->GetValue().ToStdString()) == 0) {

                // ) if nozzle_temperature_range is found
                ConfigOption* opt_min = it->config.option("nozzle_temperature_range_low");
                if (opt_min) {
                    ConfigOptionInts* opt_min_ints = dynamic_cast<ConfigOptionInts*>(opt_min);
                    if (opt_min_ints) {
                        wxString text_nozzle_temp_min = wxString::Format("%d", opt_min_ints->get_at(0));
                        m_input_nozzle_min->GetTextCtrl()->SetValue(text_nozzle_temp_min);
                    }
                }
                ConfigOption* opt_max = it->config.option("nozzle_temperature_range_high");
                if (opt_max) {
                    ConfigOptionInts* opt_max_ints = dynamic_cast<ConfigOptionInts*>(opt_max);
                    if (opt_max_ints) {
                        wxString text_nozzle_temp_max = wxString::Format("%d", opt_max_ints->get_at(0));
                        m_input_nozzle_max->GetTextCtrl()->SetValue(text_nozzle_temp_max);
                    }
                }
                ConfigOption* opt_type = it->config.option("filament_type");
                bool found_filament_type = false;
                if (opt_type) {
                    ConfigOptionStrings* opt_type_strs = dynamic_cast<ConfigOptionStrings*>(opt_type);
                    if (opt_type_strs) {
                        found_filament_type = true;
                        //m_filament_type = opt_type_strs->get_at(0);
                        std::string display_filament_type;
                        m_filament_type = it->config.get_filament_type(display_filament_type);
                    }
                }
                if (!found_filament_type)
                    m_filament_type = "";

                break;
            }
        }
    }
    if (m_input_nozzle_min->GetTextCtrl()->GetValue().IsEmpty()) {
         m_input_nozzle_min->GetTextCtrl()->SetValue("0");
    }
    if (m_input_nozzle_max->GetTextCtrl()->GetValue().IsEmpty()) {
         m_input_nozzle_max->GetTextCtrl()->SetValue("0");
    }

    m_filament_selection = evt.GetSelection();
}

void AMSMaterialsSetting::on_dpi_changed(const wxRect &suggested_rect) { this->Refresh(); }

ColorPicker::ColorPicker(wxWindow* parent, wxWindowID id, const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/)
{
    wxWindow::Create(parent, id, pos, size);

    SetSize(wxSize(FromDIP(25), FromDIP(25)));
    SetMinSize(wxSize(FromDIP(25), FromDIP(25)));
    SetMaxSize(wxSize(FromDIP(25), FromDIP(25)));

    Bind(wxEVT_PAINT, &ColorPicker::paintEvent, this);
    m_bitmap_border = create_scaled_bitmap("color_picker_border", nullptr, 25);
}

ColorPicker::~ColorPicker(){}

void ColorPicker::msw_rescale()
{
    m_bitmap_border = create_scaled_bitmap("color_picker_border", nullptr, 25);
    Refresh();
}

void ColorPicker::set_color(wxColour col)
{
    m_colour = col;
    Refresh();
}

void ColorPicker::set_colors(std::vector<wxColour> cols)
{
    m_cols = cols;
    Refresh();
}

void ColorPicker::paintEvent(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    render(dc);
}

void ColorPicker::render(wxDC& dc)
{
#ifdef __WXMSW__
    wxSize     size = GetSize();
    wxMemoryDC memdc;
    wxBitmap   bmp(size.x, size.y);
    memdc.SelectObject(bmp);
    memdc.Blit({ 0, 0 }, size, &dc, { 0, 0 });

    {
        wxGCDC dc2(memdc);
        doRender(dc2);
    }

    memdc.SelectObject(wxNullBitmap);
    dc.DrawBitmap(bmp, 0, 0);
#else
    doRender(dc);
#endif
}

void ColorPicker::doRender(wxDC& dc)
{
    wxSize     size = GetSize();

    auto radius = m_show_full?size.x / 2:size.x / 2 - FromDIP(1);
    if (m_selected) radius -= FromDIP(1);

    dc.SetPen(wxPen(m_colour));
    dc.SetBrush(wxBrush(m_colour));
    dc.DrawCircle(size.x / 2, size.x / 2, radius);

    if (m_selected) {
        dc.SetPen(wxPen(m_colour));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(size.x / 2, size.x / 2, size.x / 2);
    }

    if (m_show_full) {
        dc.SetPen(wxPen(wxColour(0x6B6B6B)));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(size.x / 2, size.x / 2, radius);

        if (m_cols.size() > 1) {
            int left = FromDIP(0);
            float total_width = size.x;
            int gwidth = std::round(total_width / (m_cols.size() - 1));

            for (int i = 0; i < m_cols.size() - 1; i++) {

                if ((left + gwidth) > (size.x)) {
                    gwidth = size.x - left;
                }

                auto rect = wxRect(left, 0, gwidth, size.y);
                dc.GradientFillLinear(rect, m_cols[i], m_cols[i + 1], wxEAST);
                left += gwidth;
            }
            dc.DrawBitmap(m_bitmap_border, wxPoint(0, 0));
        }
    }
}

ColorPickerPopup::ColorPickerPopup(wxWindow* parent)
    :PopupWindow(parent, wxBORDER_NONE)
{
    m_def_colors.clear();
    m_def_colors.push_back(wxColour(0xFFFFFF));
    m_def_colors.push_back(wxColour(0xfff144));
    m_def_colors.push_back(wxColour(0xDCF478));
    m_def_colors.push_back(wxColour(0x0ACC38));
    m_def_colors.push_back(wxColour(0x057748));
    m_def_colors.push_back(wxColour(0x0d6284));
    m_def_colors.push_back(wxColour(0x0EE2A0));
    m_def_colors.push_back(wxColour(0x76D9F4));
    m_def_colors.push_back(wxColour(0x46a8f9));
    m_def_colors.push_back(wxColour(0x2850E0));
    m_def_colors.push_back(wxColour(0x443089));
    m_def_colors.push_back(wxColour(0xA03CF7));
    m_def_colors.push_back(wxColour(0xF330F9));
    m_def_colors.push_back(wxColour(0xD4B1DD));
    m_def_colors.push_back(wxColour(0xf95d73));
    m_def_colors.push_back(wxColour(0xf72323));
    m_def_colors.push_back(wxColour(0x7c4b00));
    m_def_colors.push_back(wxColour(0xf98c36));
    m_def_colors.push_back(wxColour(0xfcecd6));
    m_def_colors.push_back(wxColour(0xD3C5A3));
    m_def_colors.push_back(wxColour(0xAF7933));
    m_def_colors.push_back(wxColour(0x898989));
    m_def_colors.push_back(wxColour(0xBCBCBC));
    m_def_colors.push_back(wxColour(0x161616));


    SetBackgroundColour(wxColour(*wxWHITE));

    wxBoxSizer* m_sizer_main = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* m_sizer_box = new wxBoxSizer(wxVERTICAL);

    m_def_color_box = new StaticBox(this);
    wxBoxSizer* m_sizer_ams = new wxBoxSizer(wxHORIZONTAL);
    auto m_title_ams = new wxStaticText(m_def_color_box, wxID_ANY, _L("AMS"), wxDefaultPosition, wxDefaultSize, 0);
    m_title_ams->SetFont(::Label::Body_14);
    m_title_ams->SetBackgroundColour(wxColour(238, 238, 238));
    m_sizer_ams->Add(m_title_ams, 0, wxALL, 5);
    auto ams_line = new wxPanel(m_def_color_box, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    ams_line->SetBackgroundColour(wxColour(0xCECECE));
    ams_line->SetMinSize(wxSize(-1, 1));
    ams_line->SetMaxSize(wxSize(-1, 1));
    m_sizer_ams->Add(ams_line, 1, wxALIGN_CENTER, 0);


    m_def_color_box->SetCornerRadius(FromDIP(10));
    m_def_color_box->SetBackgroundColor(StateColor(std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Normal)));
    m_def_color_box->SetBorderColor(StateColor(std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Normal)));

    //ams
    m_ams_fg_sizer = new wxFlexGridSizer(0, 8, 0, 0);
    m_ams_fg_sizer->SetFlexibleDirection(wxBOTH);
    m_ams_fg_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    //other
    wxFlexGridSizer* fg_sizer;
    fg_sizer = new wxFlexGridSizer(0, 8, 0, 0);
    fg_sizer->SetFlexibleDirection(wxBOTH);
    fg_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);


    for (wxColour col : m_def_colors) {
        auto cp = new ColorPicker(m_def_color_box, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        cp->set_color(col);
        cp->set_selected(false);
        cp->SetBackgroundColour(StateColor::darkModeColorFor(wxColour(238,238,238)));
        m_color_pickers.push_back(cp);
        fg_sizer->Add(cp, 0, wxALL, FromDIP(3));
        cp->Bind(wxEVT_LEFT_DOWN, [this, cp](auto& e) {
            set_def_colour(cp->m_colour);

            wxCommandEvent evt(EVT_SELECTED_COLOR);
            unsigned long g_col = ((cp->m_colour.Red() & 0xff) << 16) + ((cp->m_colour.Green() & 0xff) << 8) + (cp->m_colour.Blue() & 0xff);
            evt.SetInt(g_col);
            wxPostEvent(GetParent(), evt);
        });
    }

    wxBoxSizer* m_sizer_other = new wxBoxSizer(wxHORIZONTAL);
    auto m_title_other = new wxStaticText(m_def_color_box, wxID_ANY, _L("Other color"), wxDefaultPosition, wxDefaultSize, 0);
    m_title_other->SetFont(::Label::Body_14);
    m_title_other->SetBackgroundColour(wxColour(238, 238, 238));
    m_sizer_other->Add(m_title_other, 0, wxALL, 5);
    auto other_line = new wxPanel(m_def_color_box, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    other_line->SetMinSize(wxSize(-1, 1));
    other_line->SetMaxSize(wxSize(-1, 1));
    other_line->SetBackgroundColour(wxColour(0xCECECE));
    m_sizer_other->Add(other_line, 1, wxALIGN_CENTER, 0);

    m_sizer_box->Add(0, 0, 0, wxTOP, FromDIP(10));
    m_sizer_box->Add(m_sizer_ams, 1, wxEXPAND|wxLEFT|wxRIGHT, FromDIP(10));
    m_sizer_box->Add(m_ams_fg_sizer, 0, wxEXPAND|wxLEFT|wxRIGHT, FromDIP(10));
    m_sizer_box->Add(m_sizer_other, 1, wxEXPAND|wxLEFT|wxRIGHT, FromDIP(10));
    m_sizer_box->Add(fg_sizer, 0, wxEXPAND|wxLEFT|wxRIGHT, FromDIP(10));
    m_sizer_box->Add(0, 0, 0, wxTOP, FromDIP(10));


    m_def_color_box->SetSizer(m_sizer_box);
    m_def_color_box->Layout();
    m_def_color_box->Fit();

    m_sizer_main->Add(m_def_color_box, 0, wxALL | wxEXPAND, 10);
    SetSizer(m_sizer_main);
    Layout();
    Fit();

    Bind(wxEVT_PAINT, &ColorPickerPopup::paintEvent, this);
    wxGetApp().UpdateDarkUIWin(this);
}


void ColorPickerPopup::set_ams_colours(std::vector<wxColour> ams)
{
    if (m_ams_color_pickers.size() > 0) {
        for (ColorPicker* col_pick:m_ams_color_pickers) {

            std::vector<ColorPicker*>::iterator iter = find(m_color_pickers.begin(), m_color_pickers.end(), col_pick);
            if (iter != m_color_pickers.end()) {
                col_pick->Destroy();
                m_color_pickers.erase(iter);
            }
        }

        m_ams_color_pickers.clear();
    }


    m_ams_colors = ams;
    for (wxColour col : m_ams_colors) {
        auto cp = new ColorPicker(m_def_color_box, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        cp->set_color(col);
        cp->set_selected(false);
        cp->SetBackgroundColour(StateColor::darkModeColorFor(wxColour(238,238,238)));
        m_color_pickers.push_back(cp);
        m_ams_color_pickers.push_back(cp);
        m_ams_fg_sizer->Add(cp, 0, wxALL, FromDIP(3));
        cp->Bind(wxEVT_LEFT_DOWN, [this, cp](auto& e) {
            set_def_colour(cp->m_colour);

            wxCommandEvent evt(EVT_SELECTED_COLOR);
            unsigned long g_col = ((cp->m_colour.Red() & 0xff) << 16) + ((cp->m_colour.Green() & 0xff) << 8) + (cp->m_colour.Blue() & 0xff);
            evt.SetInt(g_col);
            wxPostEvent(GetParent(), evt);
        });
    }
    m_ams_fg_sizer->Layout();
    Layout();
    Fit();
}

void ColorPickerPopup::set_def_colour(wxColour col)
{
    m_def_col = col;

    for (ColorPicker* cp : m_color_pickers) {
        if (cp->m_selected) {
            cp->set_selected(false);
        }
    }

    for (ColorPicker* cp : m_color_pickers) {
        if (cp->m_colour == m_def_col) {
            cp->set_selected(true);
            break;
        }
    }

    Dismiss();
}

void ColorPickerPopup::paintEvent(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    dc.SetPen(wxColour(0xAC, 0xAC, 0xAC));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 0);
}

void ColorPickerPopup::OnDismiss() {}

void ColorPickerPopup::Popup() 
{
    PopupWindow::Popup();
}

bool ColorPickerPopup::ProcessLeftDown(wxMouseEvent& event) {
    return PopupWindow::ProcessLeftDown(event);
}

}} // namespace Slic3r::GUI