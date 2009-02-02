#include "FloatingCamWnd.h"

#include "gtkutil/FramedWidget.h"

#include "GlobalCamera.h"

namespace ui {

FloatingCamWnd::FloatingCamWnd(GtkWindow* parent) :
	PersistentTransientWindow("Camera", parent, true)
{
	// Pack the camera widget into this window
	_camWnd = GlobalCamera().getCamWnd();
	_camWnd->setContainer(GTK_WINDOW(getWindow()));

	gtk_container_add(
		GTK_CONTAINER(getWindow()),
		gtkutil::FramedWidget(_camWnd->getWidget())
	);
	
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_NORMAL);
}

FloatingCamWnd::~FloatingCamWnd() {
	// Disconnect the camera from its parent container
	_camWnd->setContainer(NULL);

	// GtkWindow destruction is handled by the base class destructor
}

} // namespace ui
