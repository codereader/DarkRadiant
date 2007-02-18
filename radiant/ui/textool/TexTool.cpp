#include "TexTool.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/glwidget.h"
#include "mainframe.h"

namespace ui {
	
	namespace {
		const std::string WINDOW_TITLE = "Texture Tool";
		
		const std::string RKEY_ROOT = "user/ui/textures/texTool/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	}

TexTool::TexTool()
{
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_window = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow(), false);
	
	_glWidget = glwidget_new(TRUE);
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_window), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), this);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_window));
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_window));
}

void TexTool::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_window)) {
		gtkutil::TransientWindow::minimise(_window);
		gtk_widget_hide_all(_window);
	}
	else {
		// First restore the window
		gtkutil::TransientWindow::restore(_window);
		// then apply the saved position
		_windowPosition.applyPosition();
		// Now show it
		gtk_widget_show_all(_window);
	}
}

void TexTool::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_window));
}

TexTool& TexTool::Instance() {
	static TexTool _instance;
	
	return _instance;
}

gboolean TexTool::onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self) {
	// Toggle the visibility of the textool window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}
	
} // namespace ui
