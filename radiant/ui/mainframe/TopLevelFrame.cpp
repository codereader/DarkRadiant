#include "TopLevelFrame.h"

#include "ieventmanager.h"

namespace ui
{

BEGIN_EVENT_TABLE(TopLevelFrame, wxFrame)
	EVT_MOUSEWHEEL(TopLevelFrame::redirectMouseWheelToWindowBelowCursor)
END_EVENT_TABLE()

TopLevelFrame::TopLevelFrame() :
	wxFrame(NULL, wxID_ANY, wxT("DarkRadiant")),
	_topLevelContainer(NULL),
	_mainContainer(NULL)
{
	_topLevelContainer = new wxBoxSizer(wxVERTICAL);
	SetSizer(_topLevelContainer);

	_mainContainer = new wxBoxSizer(wxVERTICAL);
	_topLevelContainer->Add(_mainContainer, 1, wxEXPAND);

	GlobalEventManager().connect(*this);
}

TopLevelFrame::~TopLevelFrame()
{
	GlobalEventManager().disconnect(*this);
}

void TopLevelFrame::redirectMouseWheelToWindowBelowCursor(wxMouseEvent& ev)
{
	wxPoint mousePos = wxGetMousePosition();
	wxWindow* windowAtPoint = wxFindWindowAtPointer(mousePos);

	if (windowAtPoint) 
	{
		windowAtPoint->GetEventHandler()->AddPendingEvent(ev);
	}
}

wxBoxSizer* TopLevelFrame::getMainContainer()
{
	return _mainContainer;
}

} // namespace
