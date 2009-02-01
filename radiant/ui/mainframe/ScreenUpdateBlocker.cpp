#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "map/AutoSaver.h"
#include "imainframe.h"

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message) :
	TransientWindow(title, GlobalRadiant().getMainWindow())
{
	gtk_window_set_resizable(GTK_WINDOW(getWindow()), FALSE);
	gtk_window_set_deletable(GTK_WINDOW(getWindow()), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 0);

	GtkWidget* label = gtkutil::LeftAlignedLabel(message);
	gtk_widget_set_size_request(label, 200, -1);

	gtk_container_add(GTK_CONTAINER(getWindow()), label);

	// Eat all window events
	gtk_grab_add(getWindow());

	// Stop the autosaver
	map::AutoSaver().stopTimer();

	// Set the "screen updates disabled" flag
	GlobalMainFrame().disableScreenUpdates();

	// Show this window immediately
	show();
}

ScreenUpdateBlocker::~ScreenUpdateBlocker() {
	// Remove the event blocker
	gtk_grab_remove(getWindow());

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

	// Start the autosaver again
	map::AutoSaver().startTimer();

	destroy();
}

} // namespace ui
