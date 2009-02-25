#ifndef OVERLAYDIALOG_H_
#define OVERLAYDIALOG_H_

#include "icommandsystem.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkrange.h>

#include "gtkutil/RegistryConnector.h"

#include <map>
#include <string>

namespace ui
{

/**
 * Dialog to configure the background image overlay options for the ortho
 * window.
 */
class OverlayDialog
{
	// Main widget
	GtkWidget* _widget;

	// Subwidgets, held in a map. This is just a named list of widgets, to 
	// avoid adding new member variables for each widget.
	typedef std::map<std::string, GtkWidget*> WidgetMap;
	WidgetMap _subWidgets;

	// The helper class that syncs the widgets with the Registry on demand
	gtkutil::RegistryConnector _connector;

	// TRUE, if a widget update is in progress (to avoid callback loops)
	bool _callbackActive;

private:

	// Constructor creates GTK widgets	
	OverlayDialog();
	
	// Widget construction helpers
	GtkWidget* createWidgets();
	GtkWidget* createButtons();
	
	/** greebo: Connects the widgets to the Registry
	 */
	void connectWidgets();
	
	// Get the overlay state from the registry, and set dialog widgets
	// accordingly
	void getStateFromRegistry();
	
	// Updates the sensitivity of the objects according to the registry state
	void updateSensitivity();
	
	// GTK callbacks
	static void _onClose(GtkWidget*, OverlayDialog*);
	static void _onUseImage(GtkToggleButton*, OverlayDialog*);
	static void _onFileSelection(GtkFileChooser*, OverlayDialog*);
	
	static void _onChange(GtkWidget*, OverlayDialog*);
	static void _onScrollChange(GtkWidget* range, OverlayDialog* self);

public:

	/**
	 * Static method to display the overlay dialog.
	 */
	static void display(const cmd::ArgumentList& args);

};

}

#endif /*OVERLAYDIALOG_H_*/
