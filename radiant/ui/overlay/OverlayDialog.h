#ifndef OVERLAYDIALOG_H_
#define OVERLAYDIALOG_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktogglebutton.h>

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

private:

	// Constructor creates GTK widgets	
	OverlayDialog();
	
	// Widget construction helpers
	GtkWidget* createWidgets();
	GtkWidget* createButtons();
	
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
