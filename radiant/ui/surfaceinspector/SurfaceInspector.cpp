#include "SurfaceInspector.h"

#include "gtkutil/TransientWindow.h"
#include "mainframe.h"

namespace ui {

	namespace {
		const std::string WINDOW_TITLE = "Surface Inspector";
	}

SurfaceInspector::SurfaceInspector() {
	_dialog = gtkutil::TransientWindow(WINDOW_TITLE, MainFrame_getWindow());
}

void SurfaceInspector::toggle() {
	gtk_widget_show_all(_dialog);
}

void SurfaceInspector::toggleInspector() {
	// The static instance
	static SurfaceInspector _inspector;

	// Now toggle the dialog
	_inspector.toggle();
}

} // namespace ui
