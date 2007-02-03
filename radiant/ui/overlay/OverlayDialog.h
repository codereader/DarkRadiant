#ifndef OVERLAYDIALOG_H_
#define OVERLAYDIALOG_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkfilechooser.h>
#include <gtk/gtkrange.h>

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

private:

	// Constructor creates GTK widgets	
	OverlayDialog();
	
	// Widget construction helpers
	GtkWidget* createWidgets();
	GtkWidget* createButtons();
	
	// Get the overlay state from the registry, and set dialog widgets
	// accordingly
	void getStateFromRegistry();
	
	// GTK callbacks
	static void _onClose(GtkWidget*, OverlayDialog*);
	static void _onUseImage(GtkToggleButton*, OverlayDialog*);
	static void _onFileSelection(GtkFileChooser*, OverlayDialog*);
	static void _onKeepAspect(GtkToggleButton*, OverlayDialog*);
	static void _onScaleImage(GtkToggleButton*, OverlayDialog*);
	static bool _onTransparencyScroll(
							GtkRange*, GtkScrollType, double, OverlayDialog*);
	static bool _onScaleScroll(
							GtkRange*, GtkScrollType, double, OverlayDialog*);

public:

	/**
	 * Static method to display the overlay dialog.
	 */
	static void display();

};

}

#endif /*OVERLAYDIALOG_H_*/
