#include "OverlayDialog.h"

#include <gtk/gtk.h>

namespace ui
{

// Create GTK stuff in c-tor
OverlayDialog::OverlayDialog()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{

}

// Static show method
void OverlayDialog::display() {
	
	// Maintain a static dialog instance and display it on demand
	static OverlayDialog _instance;
	gtk_widget_show_all(_instance._widget);
}

} // namespace ui
