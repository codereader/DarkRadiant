#include "PanedPosition.h"

#include "string/convert.h"
#include "iregistry.h"
#include <wx/splitter.h>

namespace
{
	const int DEFAULT_POSITION = 200;
}

namespace wxutil
{

PanedPosition::PanedPosition() :
	_position(DEFAULT_POSITION),
	_paned(NULL)
{}

PanedPosition::~PanedPosition()
{
	if (_paned != NULL)
	{
		_paned->Disconnect(wxEVT_SPLITTER_SASH_POS_CHANGED, 
			wxSplitterEventHandler(PanedPosition::onPositionChange), NULL, this);
	}
}

void PanedPosition::connect(wxSplitterWindow* paned)
{
	assert(_paned == NULL); // detect weirdness

	_paned = paned;

	_paned->Connect(wxEVT_SPLITTER_SASH_POS_CHANGED, 
			wxSplitterEventHandler(PanedPosition::onPositionChange), NULL, this);
}

void PanedPosition::disconnect(wxSplitterWindow* paned)
{
	if (_paned == NULL) return;

	assert(_paned == paned); // detect weirdness

	_paned->Disconnect(wxEVT_SPLITTER_SASH_POS_CHANGED, 
			wxSplitterEventHandler(PanedPosition::onPositionChange), NULL, this);

	_paned = NULL;
}

const int PanedPosition::getPosition() const
{
	return _position;
}

void PanedPosition::setPosition(int position)
{
	_position = position;
}

void PanedPosition::saveToPath(const std::string& path)
{
	GlobalRegistry().setAttribute(path, "position", string::to_string(_position));
}

void PanedPosition::loadFromPath(const std::string& path)
{
	_position = string::convert<int>(GlobalRegistry().getAttribute(path, "position"));
}

void PanedPosition::applyPosition()
{
	if (_paned != NULL)
	{
		_paned->SetSashPosition(_position, true);
	}
}

// Reads the position from the splitter window and normalises it to the paned size
void PanedPosition::readPosition()
{
	if (_paned != NULL)
	{
		_position = _paned->GetSashPosition();
	}
}

void PanedPosition::onPositionChange(wxSplitterEvent& ev)
{
	// Tell the object to read the new position from wx
	readPosition();
}

} // namespace
