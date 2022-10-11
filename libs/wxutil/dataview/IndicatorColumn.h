#pragma once

#include <wx/dataview.h>
#include <wx/dc.h>

namespace wxutil
{

class IndicatorRenderer :
    public wxDataViewCustomRenderer
{
private:
    const wxBrush* _inactiveBrush;
    const wxBrush* _activeBrush;

    bool _active;

public:
    IndicatorRenderer() :
        wxDataViewCustomRenderer("bool"),
        _inactiveBrush(wxGREY_BRUSH),
        _activeBrush(wxRED_BRUSH),
        _active(false)
    {}

    bool Render(wxRect cell, wxDC* dc, int state) override
    {
        dc->SetBrush(_active ? *_activeBrush : *_inactiveBrush);
        dc->DrawRectangle(cell);
        return true;
    }

    wxSize GetSize() const override
    {
        return wxSize(4, 16);
    }

    bool GetValue(wxVariant& value) const override
    {
        value = _active;
        return true;
    }

    bool SetValue(const wxVariant& value) override
    {
        _active = value.GetBool();
        return true;
    }
};

/**
 * Custom wxDataViewColumn rendering a 5-pixel wide rectangle indicating
 * a true (active) or false (inactive) state.
 */
class IndicatorColumn :
    public wxDataViewColumn
{
public:
    constexpr static int DEFAULT_WIDTH = 10;

    // The default width of 10 results in a 4 pixel rectangle (after deducting the 3-pixel padding on both sides)
    IndicatorColumn(const std::string& title, int modelColumn, int width = DEFAULT_WIDTH, 
                   wxAlignment align = wxALIGN_CENTER, int flags = wxDATAVIEW_COL_RESIZABLE) :
        wxDataViewColumn(title, new IndicatorRenderer(), modelColumn, width, align, flags)
    {}
};

}
