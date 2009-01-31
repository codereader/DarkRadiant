#ifndef _STATUS_BAR_MANAGER_H_
#define _STATUS_BAR_MANAGER_H_

#include "iuimanager.h"
#include <map>
#include <gtk/gtkwidget.h>

namespace ui {

class StatusBarManager :
	public IStatusBarManager
{
	struct StatusBarElement {
		// The toplevel container 
		GtkWidget* toplevel;

		// If this status bar element is a label, this is not NULL
		GtkWidget* label;

		StatusBarElement(GtkWidget* _toplevel) :
			toplevel(_toplevel),
			label(NULL)
		{}

		StatusBarElement(GtkWidget* _toplevel, GtkWidget* _label) :
			toplevel(_toplevel),
			label(_label)
		{}
	};
	typedef boost::shared_ptr<StatusBarElement> StatusBarElementPtr;

	// Editable status bar widgets
	typedef std::map<std::string, StatusBarElementPtr> ElementMap;
	ElementMap _elements;

	// Container
	typedef std::map<int, StatusBarElementPtr> PositionMap;
	PositionMap _positions;

	// The main status bar
	GtkWidget* _statusBar;

public:
	StatusBarManager();

	/**
	 * Get the status bar widget, for packing into the main window.
	 */
	GtkWidget* getStatusBar();

	/**
	 * greebo: This adds a named element to the status bar. Pass the widget
	 * which should be added and specify the position order.
	 * 
	 * @name: the name of the element (can be used for later lookup).
	 * @widget: the widget to pack.
	 * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
	 *       at the front or back of the status bar container.
	 */
	void addElement(const std::string& name, GtkWidget* widget, int pos);

	/**
	 * Returns a named status bar widget, previously added by addElement().
	 * 
	 * @returns: NULL if the named widget does not exist.
	 */
	GtkWidget* getElement(const std::string& name);

	/**
	 * greebo: A specialised method, adding a named text element.
	 * Use the setText() method to update this element.
	 *
	 * @name: the name for this element (can be used as key for the setText() method).
	 * @icon: the icon file to pack into the element, relative the BITMAPS_PATH. Leave empty
	 *        if no icon is desired.
	 * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
	 *       at the front or back of the status bar container.
 	 */
	void addTextElement(const std::string& name, const std::string& icon, int pos);

	/**
	 * Updates the content of the named text element. The name must refer to
	 * an element previously added by addTextElement().
	 */
	void setText(const std::string& name, const std::string& text);

private:
	// Returns an integer position which is not used yet.
	int getFreePosition(int desiredPosition);

	// Rebuilds the status bar after widget addition
	void rebuildStatusBar();

	// Removes all encountered widgets from the statusbar
	static void _removeChildWidgets(GtkWidget* child, gpointer statusBar);
};

} // namespace ui

#endif /* _STATUS_BAR_MANAGER_H_ */
