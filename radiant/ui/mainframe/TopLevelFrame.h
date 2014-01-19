#pragma once

#include <wx/wxprec.h>

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

public:
	TopLevelFrame();

	wxBoxSizer* getMainContainer();

private:
	DECLARE_EVENT_TABLE();
};

} // namespace
