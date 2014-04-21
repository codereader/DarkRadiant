#pragma once

#include <wx/dialog.h>
#include <wx/frame.h>
#include <wx/display.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include "imainframe.h"
#include "ScrollEventPropagationFilter.h"

namespace wxutil
{

/**
 * Base class for many DarkRadiant dialogs.
 * It comes with a panel/sizer combination pre-populated, use
 * the _mainPanel member to add more stuff.
 */
class DialogBase :
	public wxDialog
{
private:
	ScrollEventPropagationFilterPtr _scrollEventFilter;

public:
	DialogBase(const std::string& title, wxWindow* parent = NULL) :
		wxDialog(parent != NULL ? parent : GlobalMainFrame().getWxTopLevelWindow(), 
			wxID_ANY, title, wxDefaultPosition, wxDefaultSize, 
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		_scrollEventFilter(new ScrollEventPropagationFilter)
	{}

	/**
	 * Adjust this window to fit the display DarkRadiant is currently (mainly) active on.
	 * Set the xProp and yProp factors to control how much space this window should use.
	 * The factors should be in the range (0.0..1.0], where 1.0 takes the entire space.
	 */
	void FitToScreen(float xProp, float yProp)
	{
		int curDisplayIdx = wxDisplay::GetFromWindow(GlobalMainFrame().getWxTopLevelWindow());
		wxDisplay curDisplay(curDisplayIdx);

		wxRect rect = curDisplay.GetGeometry();
		int newWidth = static_cast<int>(rect.GetWidth() * xProp);
		int newHeight = static_cast<int>(rect.GetHeight() * yProp);

		SetSize(newWidth, newHeight);
		CenterOnScreen();
	}
};

} // namespace
