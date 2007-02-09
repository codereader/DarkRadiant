#ifndef FRAMEDTRANSIENTWIDGET_H_
#define FRAMEDTRANSIENTWIDGET_H_

#include <string>
#include "gtk/gtkwidget.h"
#include "gtk/gtkwindow.h"

namespace gtkutil
{

/** greebo: Encapsulation of a framed, transient Window with a frame around the contained widget
 * 
 * Pass the widget to the class constructor and use the operator GtkWidget* to retrieve 
 * the completed Window widget. 
 */

class FramedTransientWidget
{
protected:
	// The text label
	const std::string _title;
	
	// The window that this window is transient for
	GtkWindow* _parent;
	
	// The contained widget
	GtkWidget* _containedWidget;
	
public:

	// Constructor
	FramedTransientWidget(const std::string& title, 
						  GtkWindow* parent, 
						  GtkWidget* containedWidget);
	
	// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
	virtual operator GtkWidget* ();

};

} // namespace gtkutil

#endif /*FRAMEDTRANSIENTWIDGET_H_*/
