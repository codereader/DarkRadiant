#ifndef WIDGETTOGGLE_H_
#define WIDGETTOGGLE_H_

#include "ieventmanager.h"
#include "Toggle.h"

#include <vector>
#include "gtk/gtkwidget.h"

/* greebo: A WidgetToggle can be connected to one or more widgets and shows/hides them 
 * upon toggle (e.g. like the Camera Window).
 * 
 * Note: GtkCheckMenuItems and GtkToggleToolButtons can not be hidden/shown by 
 * this class as they are the "native" GtkWidgets of the Toggle class which are
 * used in the menus and toolbars. 
 * 
 * Therefore the connectWidget() method is overridden by this subclass and checks
 * for these items before adding them to the internal list.
 */
class WidgetToggle :
	public Toggle
{
	typedef std::vector<GtkWidget*> WidgetList;

	// The list of all the connected widgets
	WidgetList _widgets;

public:
	// Constructor
	WidgetToggle();

	virtual ~WidgetToggle() {}
	
	// Dummy callback for the Toggle base class, we don't need any callbacks...
	void doNothing();
	
	/* This method only adds the widget to the show/hide list if the widget
	 * is NOT of type GtkCheckMenuItem/GtkToggleToolButtons. Any other
	 * widgets are added to the show/hide list */
	virtual void connectWidget(GtkWidget* widget);
	
	virtual void updateWidgets();
	
private:

	// Show/hide all the connected widgets
	void showWidgets();
	void hideWidgets();

}; // class WidgetToggle

#endif /*WIDGETTOGGLE_H_*/
