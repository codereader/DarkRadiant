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
	// The contained widgets
	GtkWidget* _firstchild;
	GtkWidget* _secondChild;
	
	// The actual paned container
	GtkWidget* _paned;
	
public:

	// Constructor
	Paned(GtkWidget* firstchild, GtkWidget* secondChild, bool isHorizontal) : 
		_firstchild(firstchild),
		_secondChild(secondChild)
	{
		if (isHorizontal) {
			_paned = gtk_hpaned_new();
		}
		else {
			_paned = gtk_vpaned_new();
		}
		
		gtk_paned_add1(GTK_PANED(_paned), _firstchild);
        gtk_paned_add2(GTK_PANED(_paned), _secondChild);
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
