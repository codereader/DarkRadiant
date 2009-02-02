#include "Splash.h"

#include <gtk/gtk.h>
#include "gtkutil/LeftAlignedLabel.h"

#include "modulesystem/ModuleRegistry.h"
#include "gtkmisc.h" // for process_gui()

namespace ui {
	
	namespace {
		const std::string SPLASH_FILENAME = "darksplash.png";
	}

Splash::Splash() :
	_window(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL))),
	_progressBar(NULL)
{
	gtk_window_set_decorated(_window, FALSE);
	gtk_window_set_resizable(_window, FALSE);
	gtk_window_set_modal(_window, TRUE);
	gtk_window_set_default_size(_window, -1, -1);
	gtk_window_set_position(_window, GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(_window), 0);
	
	const ApplicationContext& ctx = module::getRegistry().getApplicationContext();
	std::string fullFileName(ctx.getBitmapsPath() + SPLASH_FILENAME);
	GtkWidget* image = gtk_image_new_from_pixbuf(
		gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL)
	);
	
	_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_vbox), image, TRUE, TRUE, 0);
	
	gtk_container_add(GTK_CONTAINER(_window), _vbox);
	gtk_widget_set_size_request(GTK_WIDGET(_window), -1, -1);
}

GtkWindow* Splash::getWindow() {
	return _window;
}

void Splash::createProgressBar() {
	_progressBar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(_vbox), _progressBar, FALSE, FALSE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(_window), -1, -1);
	gtk_widget_show_all(_progressBar);
}

void Splash::setText(const std::string& text) {
	if (_progressBar == NULL) {
		createProgressBar();
	}
	
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(_progressBar), text.c_str());
	queueDraw();
}

void Splash::setProgress(float fraction) {
	if (_progressBar == NULL) {
		createProgressBar();
	}
	
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(_progressBar), fraction);
	queueDraw();
}

void Splash::setProgressAndText(const std::string& text, float fraction) {
	setText(text);
	setProgress(fraction);
}

void Splash::show() {
	gtk_widget_show_all(GTK_WIDGET(_window));
	queueDraw();		
}

void Splash::hide() {
	gtk_widget_hide(GTK_WIDGET(_window));
}

void Splash::queueDraw() {
	// Trigger a (re)draw, just to make sure that it gets displayed
	gtk_widget_queue_draw(GTK_WIDGET(_window));
	process_gui();
}

Splash& Splash::Instance() {
	static Splash _instance;
	return _instance;
}

} // namespace ui
