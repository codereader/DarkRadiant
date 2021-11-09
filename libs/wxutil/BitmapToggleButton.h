#pragma once

#include <wx/tglbtn.h>

namespace wxutil
{

/**
 * @brief A toggle button which can change its bitmap when toggled.
 *
 * This is a workaround for buggy wxGTK which does not correctly change bitmaps
 * when a wxBitmapToggleButton is toggled.
 */
class BitmapToggleButton: public wxBitmapToggleButton
{
    wxBitmap _bitmapOn;
    wxBitmap _bitmapOff;

public:
    /// Construct and initialise bitmap
    BitmapToggleButton(wxWindow* parent, const wxBitmap& bitmapOn, const wxBitmap& bitmapOff)
    : wxBitmapToggleButton(parent, wxID_ANY, bitmapOff), _bitmapOn(bitmapOn), _bitmapOff(bitmapOff)
    {
        // Default to off bitmap, but change bitmaps when toggled
        SetBitmapPressed(_bitmapOn); // works on Windows but not on GTK
        Bind(wxEVT_TOGGLEBUTTON,
             [this](wxCommandEvent&) { SetBitmap(GetValue() ? _bitmapOn : _bitmapOff); });
    }
};
}