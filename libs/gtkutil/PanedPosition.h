#ifndef PANEDPOSITION_H_
#define PANEDPOSITION_H_

#include "gtk/gtkwidget.h"
#include "gtk/gtkpaned.h"
#include "xmlutil/Node.h"

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

class PanedPosition {
	// The position of this object
	int _position;
	
	// The connected paned container
	GtkPaned* _paned;

public:
	PanedPosition();

	// Connect the passed GtkPaned to this object
	void connect(GtkWidget* paned);

	const int getPosition() const;
	void setPosition(int position);
	
	void saveToNode(xml::Node node);
	
	void loadFromNode(xml::Node node);
	
	// Applies the internally stored size/position info to the GtkWindow
	// The algorithm was adapted from original GtkRadiant code (window.h) 
	void applyPosition();

	// Reads the position from the GtkPaned and normalises it to the paned size
	void readPosition();
	
private:

	// The static GTK callback that gets invoked on position change 
	static gboolean onPositionChange(GtkWidget* widget, gpointer none, PanedPosition* self);

}; // class PanedPosition

} // namespace gtkutil

#endif /*PANEDPOSITION_H_*/
