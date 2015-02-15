#include "WindowPosition.h"

#include "iregistry.h"
#include "string/convert.h"
#include "MultiMonitor.h"
#include <wx/frame.h>
#include <wx/display.h>

namespace
{
	const int DEFAULT_POSITION_X = 50;
	const int DEFAULT_POSITION_Y = 25;
	const int DEFAULT_SIZE_X = 400;
	const int DEFAULT_SIZE_Y = 300;
}

namespace wxutil
{

WindowPosition::WindowPosition() :
	_position(DEFAULT_POSITION_X, DEFAULT_POSITION_Y),
	_size(DEFAULT_SIZE_X, DEFAULT_SIZE_Y),
	_window(NULL)
{}

void WindowPosition::initialise(wxTopLevelWindow* window, 
                                const std::string& windowStateKey,
                                float defaultXFraction, 
                                float defaultYFraction)
{
    // Set up events and such
    connect(window);

    // Load from registry if possible
    if (GlobalRegistry().keyExists(windowStateKey))
    {
        loadFromPath(windowStateKey);
    }
    else
    {
        fitToScreen(defaultXFraction, defaultYFraction);
    }

    applyPosition();
}

// Connect the passed window to this object
void WindowPosition::connect(wxTopLevelWindow* window)
{
	if (_window != NULL)
	{
		disconnect(_window);
	}

	_window = window;
	applyPosition();

	window->Connect(wxEVT_SIZE, wxSizeEventHandler(WindowPosition::onResize), NULL, this);
	window->Connect(wxEVT_MOVE, wxMoveEventHandler(WindowPosition::onMove), NULL, this);
}

void WindowPosition::disconnect(wxTopLevelWindow* window)
{
	_window = NULL;

	window->Disconnect(wxEVT_SIZE, wxSizeEventHandler(WindowPosition::onResize), NULL, this);
	window->Disconnect(wxEVT_MOVE, wxMoveEventHandler(WindowPosition::onMove), NULL, this);
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
	if (_window == NULL) return;

	// TODO: What about multi-monitor setups with overlapping windows?
	wxDisplay display(wxDisplay::GetFromWindow(_window));

	// Sanity check of the window position
	if (_position[0] < 0 || _position[1] < 0 ||
		_position[0] > display.GetGeometry().GetWidth() ||
		_position[1] > display.GetGeometry().GetHeight())
	{
		_window->CenterOnParent();
	}
	else
	{
		_window->SetPosition(wxPoint(_position[0], _position[1]));
	}

	_window->SetSize(_size[0], _size[1]);
}

// Reads the position from the window
void WindowPosition::readPosition()
{
    if (_window != nullptr)
    {
        _window->GetScreenPosition(&_position[0], &_position[1]);
        _window->GetSize(&_size[0], &_size[1]);
    }
}

void WindowPosition::fitToScreen(float xfraction, float yfraction)
{
	if (_window == NULL) return;

	wxDisplay display(wxDisplay::GetFromWindow(_window));

	// Pass the call
	fitToScreen(display.GetGeometry(), xfraction, yfraction);
}

void WindowPosition::fitToScreen(const wxRect& screen, float xfraction, float yfraction)
{
	_size[0] = static_cast<int>(screen.GetWidth() * xfraction) - 12;
	_size[1] = static_cast<int>(screen.GetHeight() * yfraction) - 48;

	_position[0] = screen.GetX() + static_cast<int>((screen.GetWidth() - _size[0] - 12)/2);
	_position[1] = screen.GetY() + static_cast<int>((screen.GetHeight() - _size[1] - 48)/2);
}

void WindowPosition::onResize(wxSizeEvent& ev)
{
	setSize(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());
	ev.Skip();
}

void WindowPosition::onMove(wxMoveEvent& ev)
{
	setPosition(ev.GetPosition().x, ev.GetPosition().y);
	ev.Skip();
}

} // namespace
