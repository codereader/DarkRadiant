#include "PatchInspector.h"

#include "iregistry.h"
#include "ieventmanager.h"

#include <gtk/gtk.h>

#include "gtkutil/TransientWindow.h"

#include "mainframe.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Patch Inspector";
		
		const std::string RKEY_ROOT = "user/ui/patch/patchInspector/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

PatchInspector::PatchInspector() {
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	// Create all the widgets and pack them into the window
	populateWindow();
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_dialog));
	
	// Register self to the SelSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
	
	// Update the widget status
	update();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_dialog));
	_windowPosition.applyPosition();
}

void PatchInspector::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_dialog));
}

PatchInspector& PatchInspector::Instance() {
	// The static instance
	static PatchInspector _inspector;
	
	return _inspector;
}

void PatchInspector::populateWindow() {
	
}

void PatchInspector::update() {
	
}

void PatchInspector::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_dialog)) {
		gtkutil::TransientWindow::minimise(_dialog);
		gtk_widget_hide_all(_dialog);
	}
	else {
		gtkutil::TransientWindow::restore(_dialog);
		update();
		gtk_widget_show_all(_dialog);
	}
}

void PatchInspector::selectionChanged(scene::Instance& instance) {
	update();
}

gboolean PatchInspector::onDelete(GtkWidget* widget, GdkEvent* event, PatchInspector* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

} // namespace ui
