#pragma once

#include <memory>
#include <map>
#include "imainframe.h"
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/windowptr.h>
#include "wxutil/dialog/ScrollEventPropagationFilter.h"

class wxMenuBar;

namespace ui
{

class TopLevelFrame :
	public wxFrame
{
private:
	// Main sizer
	wxBoxSizer* _topLevelContainer;

	// The main container (where layouts can start packing stuff into)
	wxBoxSizer* _mainContainer;

    wxutil::ScrollEventPropagationFilterPtr _scrollEventFilter;

	typedef std::map<IMainFrame::Toolbar, wxWindowPtr<wxToolBar> > ToolbarMap;
	ToolbarMap _toolbars;

public:
	TopLevelFrame();

	wxBoxSizer* getMainContainer();

	wxToolBar* getToolbar(IMainFrame::Toolbar type);

private:
	wxMenuBar* createMenuBar();

	void onMenuOpenClose(wxMenuEvent& ev);
};

} // namespace
