#pragma once

#include <wx/sizer.h>

namespace wxutil
{

// Wrapper for a two-column grid layout with labels on the left and
// widget(s) on the right
class FormLayout
{
    wxWindow* _parent = nullptr;
    wxFlexGridSizer* _sizer = nullptr;

public:

    /// Initialise with parent widget
    FormLayout(wxWindow* parent): _parent(parent)
    {
        _sizer = new wxFlexGridSizer(2 /* cols */, 6 /* vgap */, 12 /* hgap */);

        // Second column expands, first (label) column is fixed width
        _sizer->AddGrowableCol(1);
    }

    /// Add a new labelled row
    void add(const std::string& label, wxSizer* widgets)
    {
        wxStaticText* text = new wxStaticText(_parent, wxID_ANY, label);
        _sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);
        _sizer->Add(widgets, 1, wxEXPAND);
    }

    /// Add a row with an empty label
    void add(wxSizer* widgets)
    {
        _sizer->AddSpacer(1);
        _sizer->Add(widgets, 1, wxEXPAND);
    }

    /// Return the populated sizer
    wxSizer* getSizer() { return _sizer; }
};

} // namespace wxutil