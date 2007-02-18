#include "TexTool.h"

#include "ieventmanager.h"
#include "iregistry.h"

#include <gtk/gtk.h>
#include <GL/glew.h>

#include "gtkutil/TransientWindow.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/GLWidgetSentry.h"
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
	
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_window), 12);
	gtk_window_set_type_hint(GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_window), "delete-event", G_CALLBACK(onDelete), this);
	
	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window
	GlobalEventManager().connect(GTK_OBJECT(_window));
	
	populateWindow();
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_window));
}

void TexTool::populateWindow() {
	// Create the GL widget
	_glWidget = glwidget_new(TRUE);
	
	// Connect the events
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(onExpose), this);
	
	// Make the GL widget accept the global shortcuts
	GlobalEventManager().connect(GTK_OBJECT(_glWidget));
	
	GtkWidget* vbox = gtk_vbox_new(true, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _glWidget, true, true, 0);
	
	gtk_container_add(GTK_CONTAINER(_window), vbox);
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
	
	GlobalEventManager().disconnect(GTK_OBJECT(_glWidget));
	GlobalEventManager().disconnect(GTK_OBJECT(_window));
}

TexTool& TexTool::Instance() {
	static TexTool _instance;
	
	return _instance;
}

void TexTool::onExpose(GtkWidget* widget, GdkEventExpose* event, TexTool* self) {
	// Make the GL widget active
	gtkutil::GLWidgetSentry sentry(self->_glWidget);
	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

gboolean TexTool::onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self) {
	// Toggle the visibility of the textool window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}
	
} // namespace ui
