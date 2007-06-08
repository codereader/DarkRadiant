#include "Splash.h"

#include <string>
#include <gtk/gtk.h>

#include "environment.h"
#include "gtkmisc.h" // for process_gui()

namespace ui {
	
	namespace {
		const std::string SPLASH_FILENAME = "darksplash.png";
	}

Splash::Splash() :
	_window(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL)))
{
	gtk_window_set_decorated(_window, FALSE);
	gtk_window_set_resizable(_window, FALSE);
	gtk_window_set_modal(_window, TRUE);
	gtk_window_set_default_size(_window, -1, -1);
	gtk_window_set_position(_window, GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(_window), 0);
	
	std::string fullFileName(Environment().Instance().getBitmapsPath() + SPLASH_FILENAME);
	GtkWidget* image = gtk_image_new_from_pixbuf(
		gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL)
	);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(_window), image);

	gtk_widget_set_size_request(GTK_WIDGET(_window), -1, -1);
}

GtkWindow* Splash::getWindow() {
	return _window;
}

void Splash::show() {
	gtk_widget_show(GTK_WIDGET(_window));
	
	// Trigger a (re)draw, just to make sure that it gets displayed
	gtk_widget_queue_draw(GTK_WIDGET(_window));
	process_gui();
}

void Splash::hide() {
	gtk_widget_hide(GTK_WIDGET(_window));
}

Splash& Splash::Instance() {
	static Splash _instance;
	return _instance;
}

} // namespace ui
