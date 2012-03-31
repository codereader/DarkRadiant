#include "WindowPosition.h"

#include "iregistry.h"
#include "string/convert.h"
#include "MultiMonitor.h"

namespace {
	const int DEFAULT_POSITION_X = 50;
	const int DEFAULT_POSITION_Y = 25;
	const int DEFAULT_SIZE_X = 400;
	const int DEFAULT_SIZE_Y = 300;
}

namespace gtkutil
{

WindowPosition::WindowPosition() :
	_position(DEFAULT_POSITION_X, DEFAULT_POSITION_Y),
	_size(DEFAULT_SIZE_X, DEFAULT_SIZE_Y)
{}

// Connect the passed window to this object
void WindowPosition::connect(Gtk::Window* window)
{
	_window = window;
	applyPosition();

	_window->signal_configure_event().connect(sigc::mem_fun(*this, &WindowPosition::onConfigure), false);
}

const WindowPosition::Position& WindowPosition::getPosition() const
{
	return _position;
}

const WindowPosition::Size& WindowPosition::getSize() const
{
	return _size;
}

void WindowPosition::setPosition(int x, int y)
{
	_position[0] = x;
	_position[1] = y;
}

void WindowPosition::setSize(int width, int height)
{
	_size[0] = width;
	_size[1] = height;
}

void WindowPosition::saveToPath(const std::string& path)
{
	GlobalRegistry().setAttribute(path, "xPosition", string::to_string(_position[0]));
	GlobalRegistry().setAttribute(path, "yPosition", string::to_string(_position[1]));
	GlobalRegistry().setAttribute(path, "width", string::to_string(_size[0]));
	GlobalRegistry().setAttribute(path, "height", string::to_string(_size[1]));
}

void WindowPosition::loadFromPath(const std::string& path)
{
	_position[0] = string::convert<int>(GlobalRegistry().getAttribute(path, "xPosition"));
	_position[1] = string::convert<int>(GlobalRegistry().getAttribute(path, "yPosition"));

	_size[0] = string::convert<int>(GlobalRegistry().getAttribute(path, "width"));
	_size[1] = string::convert<int>(GlobalRegistry().getAttribute(path, "height"));
}

// Applies the internally stored size/position info to the GtkWindow
// The algorithm was adapted from original GtkRadiant code (window.h)
void WindowPosition::applyPosition()
{
	if (_window != NULL)
	{
		_window->set_gravity(Gdk::GRAVITY_STATIC);

		// TODO: What about multi-monitor setups with overlapping windows?
		Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();

		// Sanity check of the window position
		if (_position[0] < 0 || _position[1] < 0 ||
			_position[0] > screen->get_width() ||
			_position[1] > screen->get_height())
		{
			_window->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
		}
		else
		{
			_window->move(_position[0], _position[1]);
		}

		_window->set_default_size(_size[0], _size[1]);
		_window->resize(_size[0], _size[1]);
	}
}

// Reads the position from the Gtk::Window
void WindowPosition::readPosition()
{
	_window->get_position(_position[0], _position[1]);
	_window->get_size(_size[0], _size[1]);
}

void WindowPosition::fitToScreen(float xfraction, float yfraction)
{
	if (_window == NULL) return;

	Gdk::Rectangle geom = MultiMonitor::getMonitorForWindow(*_window);

	// Pass the call
	fitToScreen(geom, xfraction, yfraction);
}

void WindowPosition::fitToScreen(Gdk::Rectangle screen, float xfraction, float yfraction)
{
	_size[0] = static_cast<int>(screen.get_width() * xfraction) - 12;
	_size[1] = static_cast<int>(screen.get_height() * yfraction) - 48;

	_position[0] = screen.get_x() + static_cast<int>((screen.get_width() - _size[0] - 12)/2);
	_position[1] = screen.get_y() + static_cast<int>((screen.get_height() - _size[1] - 48)/2);
}

// The static GTK callback that gets invoked on window size/position changes
bool WindowPosition::onConfigure(GdkEventConfigure* ev)
{
	setPosition(ev->x, ev->y);
	setSize(ev->width, ev->height);

	return false;
}

} // namespace gtkutil
