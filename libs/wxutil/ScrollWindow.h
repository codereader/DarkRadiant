#pragma once

#include <wx/scrolwin.h>

namespace wxutil
{

/**
 * ScrollWindow is extending the vanilla wxWidgets class
 * providing an additional option to prevent auto-scrolling 
 * to a focused child when the window is re-focused itself.
 * 
 * Use the SetShouldScrollToChildOnFocus() method to control
 * the behaviour.
 */
class ScrollWindow : 
	public wxScrolledWindow
{
protected:
	bool _shouldScrollToChildOnFocus;

public:
	ScrollWindow() : 
		wxScrolledWindow(),
		_shouldScrollToChildOnFocus(true)
	{}

	ScrollWindow(wxWindow* parent, wxWindowID winid = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = wxScrolledWindowStyle) :
		wxScrolledWindow(parent, winid, pos, size, style),
		_shouldScrollToChildOnFocus(true)
	{}

	// Enable/Disable the auto-scrolling to focused child widgets when this window is focused
	void SetShouldScrollToChildOnFocus(bool shouldScroll)
	{
		_shouldScrollToChildOnFocus = shouldScroll;
	}

protected:
	// Override the protected wxScrolledWindow method to return the configured value
	bool ShouldScrollToChildOnFocus(wxWindow* child) override
	{
		return _shouldScrollToChildOnFocus;
	}
};

}
