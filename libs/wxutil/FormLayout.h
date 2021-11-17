#pragma once

#include <wx/gbsizer.h>

namespace wxutil
{

// Wrapper for a two-column grid layout with labels on the left and
// widget(s) on the right
class FormLayout
{
    wxWindow* _parent = nullptr;
    wxGridBagSizer* _sizer = nullptr;

public:

    /// Initialise with parent widget
    FormLayout(wxWindow* parent): _parent(parent)
    {
        _sizer = new wxGridBagSizer(6 /* vgap */, 12 /* hgap */);
    }

    /// Add a new labelled row
    void add(const std::string& label, wxSizer* widgets)
    {
        wxStaticText* text = new wxStaticText(_parent, wxID_ANY, label);
        const int row = _sizer->GetRows();
        _sizer->Add(text, wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
        _sizer->Add(widgets, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
    }

    /// Add a row with an empty label
    void add(wxSizer* widgets)
    {
        _sizer->Add(widgets, wxGBPosition(_sizer->GetRows(), 1), wxDefaultSpan, wxEXPAND);
    }

    /// Add a widget to take up the entire width (both columns)
    void addFullWidth(wxWindow* window)
    {
        _sizer->Add(window, wxGBPosition(_sizer->GetRows(), 0), wxGBSpan(1, 2),
                    wxEXPAND | wxTOP | wxBOTTOM, 6);
    }

    /// Return the populated sizer
    wxSizer* getSizer()
    {
        // Set the second column to growable (we cannot do this on construction,
        // because an empty wxGridBagSizer does not have any defined columns)
        if (!_sizer->IsColGrowable(1))
            _sizer->AddGrowableCol(1);

        return _sizer;
    }
};

} // namespace wxutil