#include "BindToolDialog.h"

#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <fmt/format.h>
#include <functional>

namespace ui
{

namespace
{
    const char* const TOOLMAPPING_WINDOW_TITLE = N_("Select new Binding: {0}");
}

BindToolDialog::BindToolDialog(wxWindow* parent, IMouseToolGroup& group, const MouseToolPtr& tool) :
    DialogBase(fmt::format(_(TOOLMAPPING_WINDOW_TITLE), tool->getDisplayName())),
    _selectedState(wxutil::MouseButton::NONE | wxutil::Modifier::NONE),
    _group(group),
    _tool(tool),
    _clickArea(nullptr),
    _clickPanel(nullptr)
{
    populateWindow();
    
    Layout();
    Fit();
    CenterOnParent();
}

unsigned int BindToolDialog::getChosenMouseButtonState()
{
    return _selectedState;
}

void BindToolDialog::populateWindow()
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    wxStaticText* text = new wxStaticText(this, wxID_ANY, 
        _("Please select a new button/modifier combination\nby clicking on the area below."));

    _clickPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    _clickPanel->SetBackgroundColour(wxColour(150, 150, 150));
    _clickPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

    _clickArea = new wxStaticText(_clickPanel, wxID_ANY,
        _("Click here to assign"));

    _clickPanel->GetSizer()->Add(_clickArea, 0, wxALIGN_CENTER | wxALL, 24);
    _clickPanel->Layout();

    _clickPanel->Bind(wxEVT_LEFT_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
    _clickPanel->Bind(wxEVT_RIGHT_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
    _clickPanel->Bind(wxEVT_MIDDLE_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));

    _clickArea->Bind(wxEVT_LEFT_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
    _clickArea->Bind(wxEVT_RIGHT_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
    _clickArea->Bind(wxEVT_MIDDLE_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));

#if wxCHECK_VERSION(3,0,0)
    _clickPanel->Bind(wxEVT_AUX1_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
    _clickPanel->Bind(wxEVT_AUX2_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));

    _clickArea->Bind(wxEVT_AUX1_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
    _clickArea->Bind(wxEVT_AUX2_DOWN, std::bind(&BindToolDialog::onClick, this, std::placeholders::_1));
#endif

    GetSizer()->Add(text, 0, wxALIGN_LEFT | wxALL, 6);
    GetSizer()->Add(_clickPanel, 0, wxALIGN_CENTER);
    GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxALL, 12);

    SetAffirmativeId(wxID_OK);
}

void BindToolDialog::onClick(wxMouseEvent& ev)
{
    _selectedState = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);

    std::string clickedString = wxutil::Modifier::GetModifierString(_selectedState);
    clickedString += !clickedString.empty() ? "-" : "";
    clickedString += wxutil::MouseButton::GetButtonString(_selectedState);

    _clickArea->SetLabel(clickedString);

    _clickPanel->Layout();
    Layout();
    Fit();
}

}
