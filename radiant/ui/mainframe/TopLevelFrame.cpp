#include "TopLevelFrame.h"

namespace ui
{

BEGIN_EVENT_TABLE(TopLevelFrame, wxFrame)
	//EVT_PAINT(TopLevelFrame::paintEvent)
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
}

wxBoxSizer* TopLevelFrame::getMainContainer()
{
	return _mainContainer;
}

} // namespace
