#include "BindToolDialog.h"

#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <boost/format.hpp>

namespace ui
{

namespace
{
    const int BINDTOOLDIALOG_DEFAULT_SIZE_X = 300;
    const int BINDTOOLDIALOG_DEFAULT_SIZE_Y = 250;
    const char* const TOOLMAPPING_WINDOW_TITLE = N_("Select new Binding: %s");
}

BindToolDialog::BindToolDialog(wxWindow* parent, IMouseToolGroup& group, const MouseToolPtr& tool) :
    DialogBase((boost::format(_(TOOLMAPPING_WINDOW_TITLE)) % tool->getDisplayName()).str()),
    _selectedState(wxutil::MouseButton::NONE | wxutil::Modifier::NONE),
    _group(group),
    _tool(tool)
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

    wxPanel* clickPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    clickPanel->SetBackgroundColour(wxColour(100, 100, 100));
    clickPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

    wxStaticText* clickArea = new wxStaticText(clickPanel, wxID_ANY,
        _("Click here to assign"));

    clickPanel->GetSizer()->Add(clickArea, 0, wxALL, 24);
    clickPanel->Layout();

    GetSizer()->Add(text, 0, wxALIGN_LEFT | wxALL, 6);
    GetSizer()->Add(clickPanel, 0, wxALIGN_CENTER);
    GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxALL, 12);

    SetAffirmativeId(wxID_OK);
}

}
