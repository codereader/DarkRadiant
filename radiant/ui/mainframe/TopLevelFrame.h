#pragma once

#include <memory>
#include <map>
#include "imainframe.h"
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/windowptr.h>

class wxMenuBar;

namespace ui
{

class KeyEventPropagationFilter;

class TopLevelFrame :
	public wxFrame
{
private:
	// Main sizer
	wxBoxSizer* _topLevelContainer;

	// The main container (where layouts can start packing stuff into)
	wxBoxSizer* _mainContainer;

	std::shared_ptr<KeyEventPropagationFilter> _keyEventFilter;

	typedef std::map<IMainFrame::Toolbar, wxWindowPtr<wxToolBar> > ToolbarMap;
	ToolbarMap _toolbars;

public:
	TopLevelFrame();

	wxBoxSizer* getMainContainer();

	wxToolBar* getToolbar(IMainFrame::Toolbar type);

private:
	void redirectMouseWheelToWindowBelowCursor(wxMouseEvent& ev);
	wxMenuBar* createMenuBar();
	
	DECLARE_EVENT_TABLE();
};

} // namespace
