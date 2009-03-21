#ifndef PANED_H_
#define PANED_H_

#include "gtk/gtkwidget.h"
#include "gtk/gtkhpaned.h"
#include "gtk/gtkvpaned.h"

namespace gtkutil
{

/** greebo: Encapsulation to create a paned (horizontal or vertical) container 
 * 
 * Pass the two contained widgets to the class constructor 
 * and use the operator GtkWidget* to retrieve the completed framed widget.
 * 
 * Pass TRUE to the constructor to create a horizontal pane, or FALSE otherwise.
 */
class Paned
{
	// The paned container
	GtkWidget* _paned;
	
public:

	// Constructor
	Paned(GtkWidget* firstChild, GtkWidget* secondChild, bool isHorizontal)
	{
		if (isHorizontal) {
			_paned = gtk_hpaned_new();
		}
		else {
			_paned = gtk_vpaned_new();
		}
		
        // Pack in the children
		gtk_paned_pack1(GTK_PANED(_paned), firstChild, TRUE, FALSE);
        gtk_paned_pack2(GTK_PANED(_paned), secondChild, TRUE, FALSE);
	}
	
	// Operator cast to GtkWindow*
	virtual operator GtkWidget* () {
		// Show the whole widget tree
		gtk_widget_show_all(GTK_WIDGET(_paned));
		
		// Return the readily fabricated widget
		return _paned;
	}
};
	
} // namespace gtkutil

#endif /*PANED_H_*/
