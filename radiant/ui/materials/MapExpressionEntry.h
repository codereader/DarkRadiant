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
    wxWindow* _windowToPlaceDialogsOn;

public:
    // Create a map expression entry box featuring a browse/select button
    // The windowToPlaceDialogsOn argument will be used to position the modal dialog (if set)
    MapExpressionEntry(wxWindow* parent, wxWindow* windowToPlaceDialogsOn = nullptr) :
        wxPanel(parent, wxID_ANY),
        _windowToPlaceDialogsOn(windowToPlaceDialogsOn)
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

    wxTextCtrl* GetTextCtrl()
    {
        return _textEntry;
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
    bool isLookingForCubeMap()
    {
        return GetValue().StartsWith("env/");
    }

    void onBrowseButtonClick(wxCommandEvent& ev)
    {
        auto selector = new ImageFileSelector(this, _textEntry);

        selector->SetVisibleTextureTypes(isLookingForCubeMap() ? 
            ImageFileSelector::TextureType::CubeMap : ImageFileSelector::TextureType::Map);

        if (_windowToPlaceDialogsOn != nullptr)
        {
            selector->SetPosition(_windowToPlaceDialogsOn->GetScreenPosition());
            selector->SetSize(_windowToPlaceDialogsOn->GetSize());
        }

        selector->ShowModal(_textEntry->GetValue().ToStdString());
        selector->Destroy();
    }
};

}
