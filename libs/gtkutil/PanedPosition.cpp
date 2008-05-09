#include "PanedPosition.h"

#include "gtk/gtkvpaned.h"
#include "gtk/gtkhpaned.h"
#include "string/string.h"

namespace {
	const int DEFAULT_POSITION = 200;
}

namespace gtkutil
{

PanedPosition::PanedPosition() : 
	_position(DEFAULT_POSITION)
{}

// Connect the passed GtkPaned to this object
void PanedPosition::connect(GtkWidget* paned) {
	_paned = GTK_PANED(paned);
	g_signal_connect(G_OBJECT(_paned), "notify::position", G_CALLBACK(onPositionChange), this);
}

const int PanedPosition::getPosition() const {
	return _position;
}

void PanedPosition::setPosition(int position) {
	_position = position;
}

void PanedPosition::saveToNode(xml::Node node) {
	node.setAttributeValue("position", intToStr(_position));
}

void PanedPosition::loadFromNode(xml::Node node) {
	_position = strToInt(node.getAttributeValue("position"));
}

// Applies the internally stored size/position info to the GtkWindow
// The algorithm was adapted from original GtkRadiant code (window.h) 
void PanedPosition::applyPosition() {
	if (_paned != NULL) {
		gtk_paned_set_position(_paned, _position);
	}
}

// Reads the position from the GtkPaned and normalises it to the paned size
void PanedPosition::readPosition() {
	if (_paned != NULL) {
		_position = gtk_paned_get_position(_paned);
	}
}

// The static GTK callback that gets invoked on position change 
gboolean PanedPosition::onPositionChange(GtkWidget* widget, gpointer none, PanedPosition* self) {

	// Tell the object to read the new position from GTK	
	self->readPosition();

	return FALSE;
}
	
} // namespace gtkutil
