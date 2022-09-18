#include "WindowPosition.h"

#include "iregistry.h"
#include "string/convert.h"
#include <wx/frame.h>
#include <wx/display.h>

namespace
{
	constexpr int DEFAULT_POSITION_X = 50;
	constexpr int DEFAULT_POSITION_Y = 25;
	constexpr int DEFAULT_SIZE_X = 400;
	constexpr int DEFAULT_SIZE_Y = 300;
}

namespace wxutil
{

WindowPosition::WindowPosition() :
    _position({ DEFAULT_POSITION_X, DEFAULT_POSITION_Y }),
    _size({ DEFAULT_SIZE_X, DEFAULT_SIZE_Y }),
	_window(nullptr)
{}

void WindowPosition::initialise(wxTopLevelWindow* window, const std::string& windowStateKey)
{
    initialise(window, windowStateKey, 0.6f, 0.8f);
}

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
	if (_window != nullptr)
	{
		disconnect(_window);
	}

	_window = window;
	applyPosition();

	window->Bind(wxEVT_SIZE, &WindowPosition::onResize, this);
	window->Bind(wxEVT_MOVE, &WindowPosition::onMove, this);
}

void WindowPosition::disconnect(wxTopLevelWindow* window)
{
	_window = nullptr;

	window->Unbind(wxEVT_SIZE, &WindowPosition::onResize, this);
	window->Unbind(wxEVT_MOVE, &WindowPosition::onMove, this);
}

const std::array<int, 2>& WindowPosition::getPosition() const
{
	return _position;
}

const std::array<int, 2>& WindowPosition::getSize() const
{
	return _size;
}

void WindowPosition::setPosition(int x, int y)
{
	_position.at(0) = x;
	_position.at(1) = y;
}

void WindowPosition::setSize(int width, int height)
{
	_size.at(0) = width;
	_size.at(1) = height;
}

void WindowPosition::saveToPath(const std::string& path)
{
    if (path.empty()) return;

	GlobalRegistry().setAttribute(path, "xPosition", string::to_string(_position.at(0)));
	GlobalRegistry().setAttribute(path, "yPosition", string::to_string(_position.at(1)));
	GlobalRegistry().setAttribute(path, "width", string::to_string(_size.at(0)));
	GlobalRegistry().setAttribute(path, "height", string::to_string(_size.at(1)));
}

void WindowPosition::loadFromPath(const std::string& path)
{
    if (path.empty()) return;

	_position.at(0) = string::convert<int>(GlobalRegistry().getAttribute(path, "xPosition"));
	_position.at(1) = string::convert<int>(GlobalRegistry().getAttribute(path, "yPosition"));

	_size.at(0) = string::convert<int>(GlobalRegistry().getAttribute(path, "width"));
	_size.at(1) = string::convert<int>(GlobalRegistry().getAttribute(path, "height"));

    if (_size.at(0) == 0 || _size.at(1) == 0)
    {
        auto defaultXFraction = string::convert<float>(GlobalRegistry().getAttribute(path, "defaultWidthFraction"));
        auto defaultYFraction = string::convert<float>(GlobalRegistry().getAttribute(path, "defaultHeightFraction"));

        fitToScreen(defaultXFraction, defaultYFraction);
    }

    applyPosition();
}

void WindowPosition::applyPosition()
{
	if (_window == nullptr) return;

    if (_size.at(0) == 0 || _size.at(1) == 0)
    {
        // Don't apply empty sizes
        return;
    }

	// On multi-monitor setups, wxWidgets offers a virtual big screen with
	// coordinates going from 0,0 to whatever lower-rightmost point there is

	// Sanity check the window position
    wxRect targetPos(_position.at(0), _position.at(1), _size.at(0), _size.at(1));
	
	constexpr int TOL = 8;

	// Employ a few pixels tolerance to allow for placement very near the borders
	if (wxDisplay::GetFromPoint(targetPos.GetTopLeft() + wxPoint(TOL, TOL)) == wxNOT_FOUND)
	{
		// Window probably ends up invisible, refuse these coords
		_window->CenterOnParent();
	}
	else
	{
		_window->SetPosition(wxPoint(_position.at(0), _position.at(1)));
	}

	_window->SetSize(_size.at(0), _size.at(1));
}

// Reads the position from the window
void WindowPosition::readPosition()
{
    if (_window != nullptr)
    {
        _window->GetScreenPosition(&_position.at(0), &_position.at(1));
        _window->GetSize(&_size.at(0), &_size.at(1));
    }
}

void WindowPosition::fitToScreen(float xfraction, float yfraction)
{
	if (_window == nullptr) return;

	wxDisplay display(wxDisplay::GetFromWindow(_window));

	// Pass the call
	fitToScreen(display.GetGeometry(), xfraction, yfraction);
}

void WindowPosition::fitToScreen(const wxRect& screen, float xfraction, float yfraction)
{
	_size.at(0) = static_cast<int>(screen.GetWidth() * xfraction) - 12;
	_size.at(1) = static_cast<int>(screen.GetHeight() * yfraction) - 48;

    _position.at(0) = screen.GetX() + (screen.GetWidth() - _size.at(0) - 12) / 2;
    _position.at(1) = screen.GetY() + (screen.GetHeight() - _size.at(1) - 48) / 2;
}

void WindowPosition::onResize(wxSizeEvent& ev)
{
	setSize(ev.GetSize().GetWidth(), ev.GetSize().GetHeight());
	ev.Skip();
}

void WindowPosition::onMove(wxMoveEvent& ev)
{
    if (_window == nullptr) return;

    // The position passed in the wxMoveEvent seems (on my Win10 system) 
    // to be off by about x=8,y=51
    // Call GetScreenPosition to get the real coordinates
	//setPosition(ev.GetPosition().x, ev.GetPosition().y);
    _window->GetScreenPosition(&_position.at(0), &_position.at(1));

	ev.Skip();
}

} // namespace
