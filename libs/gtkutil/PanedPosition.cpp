#include "PanedPosition.h"

#include "string/convert.h"
#include "iregistry.h"

namespace
{
	const int DEFAULT_POSITION = 200;
}

namespace gtkutil
{

PanedPosition::PanedPosition() :
	_position(DEFAULT_POSITION),
	_paned(NULL)
{}

PanedPosition::~PanedPosition()
{
	if (_paned != NULL)
	{
		_connection.disconnect();
	}
}

void PanedPosition::connect(Gtk::Paned* paned)
{
	_paned = paned;

	_connection = _paned->connect_property_changed_with_return("position",
		sigc::mem_fun(*this, &PanedPosition::onPositionChange));
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
		_paned->set_position(_position);
	}
}

void PanedPosition::applyMinPosition()
{
	if (_paned == NULL) return;

	setPosition(_paned->property_min_position());
	applyPosition();
}

void PanedPosition::applyMaxPosition()
{
	if (_paned == NULL) return;

	setPosition(_paned->property_max_position());
	applyPosition();
}

// Reads the position from the GtkPaned and normalises it to the paned size
void PanedPosition::readPosition()
{
	if (_paned != NULL)
	{
		_position = _paned->get_position();
	}
}

void PanedPosition::onPositionChange()
{
	// Tell the object to read the new position from GTK
	readPosition();
}

} // namespace gtkutil
