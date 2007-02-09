#ifndef PANEDPOSITION_H_
#define PANEDPOSITION_H_

#include "gtk/gtkwidget.h"
#include "gtk/gtkpaned.h"
#include "gtk/gtkhpaned.h"
#include "math/Vector2.h"
#include "xmlutil/Node.h"
#include "string/string.h"

#include <iostream>

/* greebo: A PanedPosition object keeps track of the divider position.  
 * 
 * Use the connect() method to connect a GtkPaned to this object.
 * 
 * Use the loadFromNode() and saveToNode() methods to save the internal
 * size info into the given xml::Node
 * 
 * This is used by the Splitpane view (mainframe) to store the size/position of the
 * paned views on shutdown.
 */
namespace gtkutil {

	namespace {
		const int DEFAULT_POSITION = 200;
	}

class PanedPosition {
	// The position of this object
	int _position;
	
	// The connected paned container
	GtkPaned* _paned;

public:
	PanedPosition() : 
		_position(DEFAULT_POSITION)
	{}

	// Connect the passed GtkPaned to this object
	void connect(GtkWidget* paned) {
		_paned = GTK_PANED(paned);
		g_signal_connect(G_OBJECT(_paned), "notify::position", G_CALLBACK(onPositionChange), this);
	}

	const int getPosition() const {
		return _position;
	}
	
	void setPosition(const int& position) {
		_position = position;
	}
	
	void saveToNode(xml::Node node) {
		node.setAttributeValue("position", intToStr(_position));
	}
	
	void loadFromNode(xml::Node node) {
		_position = strToInt(node.getAttributeValue("position"));
	}
	
	// Applies the internally stored size/position info to the GtkWindow
	// The algorithm was adapted from original GtkRadiant code (window.h) 
	void applyPosition() {
		if (_paned != NULL) {
			gtk_paned_set_position(_paned, _position);
		}
	}

	// Reads the position from the GtkPaned and normalises it to the paned size
	void readPosition() {
		if (_paned != NULL) {
			_position = gtk_paned_get_position(_paned);
		}
	}
	
private:

	// The static GTK callback that gets invoked on position change 
	static gboolean onPositionChange(GtkWidget* widget, gpointer none, PanedPosition* self) {
	
		// Tell the object to read the new position from GTK	
		self->readPosition();

		return FALSE;
	}

}; // class PanedPosition

} // namespace gtkutil

#endif /*PANEDPOSITION_H_*/
