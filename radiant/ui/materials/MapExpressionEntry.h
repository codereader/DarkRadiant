#pragma once

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>

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
};

}
