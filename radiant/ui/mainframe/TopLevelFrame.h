#pragma once

#include <memory>
#include <wx/wxprec.h>

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

public:
	TopLevelFrame();

	~TopLevelFrame();

	wxBoxSizer* getMainContainer();

private:
	void redirectMouseWheelToWindowBelowCursor(wxMouseEvent& ev);
	wxMenuBar* createMenuBar();

	DECLARE_EVENT_TABLE();
};

} // namespace
