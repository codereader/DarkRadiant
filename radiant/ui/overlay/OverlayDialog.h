#ifndef OVERLAYDIALOG_H_
#define OVERLAYDIALOG_H_

#include "icommandsystem.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/RegistryConnector.h"

#include <map>
#include <string>

namespace Gtk
{
	class Widget;
}

namespace ui
{

class OverlayDialog;
typedef boost::shared_ptr<OverlayDialog> OverlayDialogPtr;

/**
 * Dialog to configure the background image overlay options for the ortho
 * window.
 */
class OverlayDialog :
	public gtkutil::PersistentTransientWindow
{
	// Subwidgets, held in a map. This is just a named list of widgets, to
	// avoid adding new member variables for each widget.
	typedef std::map<std::string, Gtk::Widget*> WidgetMap;
	WidgetMap _subWidgets;

	// The helper class that syncs the widgets with the Registry on demand
	gtkutil::RegistryConnector _connector;

	// TRUE, if a widget update is in progress (to avoid callback loops)
	bool _callbackActive;

private:
	// Constructor creates GTK widgets
	OverlayDialog();

	// Widget construction helpers
	Gtk::Widget& createWidgets();
	Gtk::Widget& createButtons();

	/** greebo: Connects the widgets to the Registry
	 */
	void connectWidgets();

	// Get the overlay state from the registry, and set dialog widgets
	// accordingly
	void getStateFromRegistry();

	// Updates the sensitivity of the objects according to the registry state
	void updateSensitivity();

	// gtkmm callbacks
	void _onClose();
	void _onFileSelection();
	void _onChange();
	void _onScrollChange();

	// Contains the pointer to the singleton instance
	static OverlayDialogPtr& InstancePtr();

public:
	/**
	 * Static method to display the overlay dialog.
	 */
	static void display(const cmd::ArgumentList& args);

	// Called at shutdown to free the instance
	static void destroy();
};

}

#endif /*OVERLAYDIALOG_H_*/
