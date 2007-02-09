#include "WindowPosition.h"

#include "string/string.h"

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
void WindowPosition::connect(GtkWindow* window) {
	_window = window;
	applyPosition();
	g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(onConfigure), this);
}

const PositionVector WindowPosition::getPosition() const {
	return _position;
}

const SizeVector WindowPosition::getSize() const {
	return _size;
}

void WindowPosition::setPosition(const int& x, const int& y) {
	_position[0] = x;
	_position[1] = y;
}

void WindowPosition::setSize(const int& width, const int& height) {
	_size[0] = width;
	_size[1] = height;
}

void WindowPosition::saveToNode(xml::Node node) {
	node.setAttributeValue("xPosition", intToStr(_position[0]));
	node.setAttributeValue("yPosition", intToStr(_position[1]));
	node.setAttributeValue("width", intToStr(_size[0]));
	node.setAttributeValue("height", intToStr(_size[1]));
}

void WindowPosition::loadFromNode(xml::Node node) {
	_position[0] = strToInt(node.getAttributeValue("xPosition"));
	_position[1] = strToInt(node.getAttributeValue("yPosition"));
	_size[0] = strToInt(node.getAttributeValue("width"));
	_size[1] = strToInt(node.getAttributeValue("height"));
}

// Applies the internally stored size/position info to the GtkWindow
// The algorithm was adapted from original GtkRadiant code (window.h) 
void WindowPosition::applyPosition() {
	if (_window != NULL) {
		gtk_window_set_gravity(_window, GDK_GRAVITY_STATIC);

		GdkScreen* screen = gdk_screen_get_default();
		
		// Sanity check of the window position
		if (_position[0] < 0 || _position[1] < 0 || 
			_position[0] > gdk_screen_get_width(screen) || 
			_position[1] > gdk_screen_get_height(screen))
		{
			gtk_window_set_position(_window, GTK_WIN_POS_CENTER_ON_PARENT);
		}
		else {
			gtk_window_move(_window, _position[0], _position[1]);
		}

		gtk_window_set_default_size(_window, _size[0], _size[1]);
		gtk_window_resize(_window, _size[0], _size[1]);
	}
}

// Reads the position from the GtkWindow
void WindowPosition::readPosition() {
	//gtk_window_set_gravity(_window, GDK_GRAVITY_STATIC);
	gtk_window_get_position(_window, &_position[0], &_position[1]);
	gtk_window_get_size(_window, &_size[0], &_size[1]);
}

// The static GTK callback that gets invoked on window size/position changes
gboolean WindowPosition::onConfigure(GtkWidget* widget, GdkEventConfigure *event, WindowPosition* self) {
	
	self->setPosition(event->x, event->y);
	self->setSize(event->width, event->height);

	return FALSE;
}

} // namespace gtkutil
