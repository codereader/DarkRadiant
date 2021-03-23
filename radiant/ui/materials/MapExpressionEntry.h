#pragma once

#include "i18n.h"
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>

#include "ui/common/ImageFileSelector.h"

namespace ui
{

class MapExpressionEntry :
    public wxPanel
{
private:
    wxTextCtrl* _textEntry;
    wxButton* _browseButton;

public:
    MapExpressionEntry(wxWindow* parent) :
        wxPanel(parent, wxID_ANY)
    {
        SetSizer(new wxBoxSizer(wxHORIZONTAL));

        _textEntry = new wxTextCtrl(this, wxID_ANY);

        _browseButton = new wxButton(this, wxID_ANY, "...");
        _browseButton->SetMaxSize(wxSize(40, -1));
        _browseButton->Bind(wxEVT_BUTTON, &MapExpressionEntry::onBrowseButtonClick, this);
        _browseButton->SetToolTip(_("Select an Image File"));

        GetSizer()->Add(_textEntry, 1, wxRIGHT|wxALIGN_CENTER_VERTICAL, 6);
        GetSizer()->Add(_browseButton, 0, wxALIGN_CENTER_VERTICAL, 0);
    }

    wxString GetValue()
    {
        return _textEntry->GetValue();
    }

    void SetValue(const wxString& value)
    {
        _textEntry->SetValue(value);
    }

private:
    void onBrowseButtonClick(wxCommandEvent& ev)
    {
        auto selector = new ImageFileSelector(this, _textEntry);
        selector->ShowModal();
        selector->Destroy();
    }
};

}
