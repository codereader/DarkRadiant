#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "map/AutoSaver.h"
#include "imainframe.h"

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message) :
	TransientWindow(title, GlobalMainFrame().getTopLevelWindow()),
	_grabbedFocus(false),
	_focusHandler(0)
{
	gtk_window_set_resizable(GTK_WINDOW(getWindow()), FALSE);
	gtk_window_set_deletable(GTK_WINDOW(getWindow()), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 6);

	GtkWidget* label = gtkutil::LeftAlignedLabel(message);
	gtk_widget_set_size_request(label, 200, -1);

	gtk_container_add(GTK_CONTAINER(getWindow()), label);

	// Stop the autosaver
	map::AutoSaver().stopTimer();

	// Set the "screen updates disabled" flag
	GlobalMainFrame().disableScreenUpdates();

	// Connect the realize signal to remove the window decorations
	g_signal_connect(G_OBJECT(getWindow()), "realize", G_CALLBACK(onRealize), this);

	if (isActiveApp()) {
		// Show this window immediately
		show();

		// Eat all window events
		gtk_grab_add(getWindow());

		_grabbedFocus = true;

		// Process pending events to fully show the dialog
		while (gtk_events_pending()) {
			gtk_main_iteration();
		}
	}

	// Register for the "is-active" changed event, to display this dialog
	// as soon as Radiant is getting the focus again
	_focusHandler = g_signal_connect(
		G_OBJECT(GlobalMainFrame().getTopLevelWindow()), 
		"notify::is-active", 
		G_CALLBACK(onMainWindowFocus), 
		this
	);
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	// Process pending events to flush keystroke buffer etc.
	while (gtk_events_pending())
	{
		gtk_main_iteration();
	}

	// Remove the signal handler again
	if (_focusHandler != 0) {
		g_signal_handler_disconnect(G_OBJECT(GlobalMainFrame().getTopLevelWindow()), _focusHandler);
	}

	// Remove the event blocker, if appropriate
	if (_grabbedFocus) {
		gtk_grab_remove(getWindow());
	}

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

	// Start the autosaver again
	map::AutoSaver().startTimer();

	destroy();
}

bool ScreenUpdateBlocker::isActiveApp() {
	// Iterate over all top-level windows and check if any of them has focus
	for (GList* i = gtk_window_list_toplevels(); i != 0; i = g_list_next(i)) {
		if (gtk_window_is_active(GTK_WINDOW(i->data))) {
			return true;
		}
	}

	return false;
}

void ScreenUpdateBlocker::onMainWindowFocus(GtkWindow* mainWindow, void* dummy, ScreenUpdateBlocker* self) {
	// The Radiant main window has changed its active state, let's see if it became active
	// and if yes, show this blocker dialog.
	if (gtk_window_is_active(mainWindow) && !GTK_WIDGET_VISIBLE(self->getWindow())) {
		gtk_widget_show(self->getWindow());
	}
}

void ScreenUpdateBlocker::onRealize(GtkWidget* widget, ScreenUpdateBlocker* self) {
	// Disable some decorations
	gdk_window_set_decorations(
		widget->window, 
		(GdkWMDecoration)(GDK_DECOR_ALL|GDK_DECOR_MENU|GDK_DECOR_MINIMIZE|GDK_DECOR_MAXIMIZE)
	);
}

} // namespace ui
