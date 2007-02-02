#ifndef OVERLAYDIALOG_H_
#define OVERLAYDIALOG_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktogglebutton.h>

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

	// Subwidgets
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

public:

	/**
	 * Static method to display the overlay dialog.
	 */
	static void display();

};

}

#endif /*OVERLAYDIALOG_H_*/
