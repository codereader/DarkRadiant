#include "FilterDialog.h"

#include "iradiant.h"

namespace ui {

	namespace {
		const int DEFAULT_SIZE_X = 600;
	    const int DEFAULT_SIZE_Y = 550;
	   	const std::string WINDOW_TITLE = "Filter Settings";
	}

FilterDialog::FilterDialog() :
	BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow())
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Show the window and its children, enter the main loop
	show();
}

void FilterDialog::showDialog() {
	// Instantiate a new instance, blocks GTK
	FilterDialog instance;
}

} // namespace ui
