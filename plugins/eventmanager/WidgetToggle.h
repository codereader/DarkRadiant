#ifndef WIDGETTOGGLE_H_
#define WIDGETTOGGLE_H_

#include "ieventmanager.h"
#include "Toggle.h"

#include <sigc++/connection.h>
#include <map>
namespace Gtk { class Widget; }

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
	typedef std::map<Gtk::Widget*, sigc::connection> WidgetMap;

	// The list of all the connected widgets
	WidgetMap _widgets;

public:
	// Constructor
	WidgetToggle();

	/* This method only adds the widget to the show/hide list if the widget
	 * is NOT of type GtkCheckMenuItem/GtkToggleToolButtons. Any other
	 * widgets are added to the show/hide list */
	virtual void connectWidget(Gtk::Widget* widget);
	virtual void disconnectWidget(Gtk::Widget* widget);

	virtual void updateWidgets();

private:
	void doNothing(bool) {}

	// Show/hide all the connected widgets
	void showWidgets();
	void hideWidgets();

	void readToggleStateFromWidgets();

	void visibilityChanged();
	void onVisibilityChange(Gtk::Widget* widget);


}; // class WidgetToggle

#endif /*WIDGETTOGGLE_H_*/
