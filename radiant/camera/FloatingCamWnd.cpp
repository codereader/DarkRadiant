#include "FloatingCamWnd.h"

#include "gtkutil/FramedWidget.h"

#include "GlobalCamera.h"

FloatingCamWnd::FloatingCamWnd(GtkWindow* parent) :
	PersistentTransientWindow("Camera", parent, true)
{
	CamWnd::setContainer(GTK_WINDOW(getWindow()));

	gtk_container_add(
		GTK_CONTAINER(getWindow()),
		gtkutil::FramedWidget(CamWnd::getWidget())
	);
	
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_NORMAL);
}

FloatingCamWnd::~FloatingCamWnd() {
	// Disconnect the camera from its parent container
	CamWnd::setContainer(NULL);

	// GtkWindow destruction is handled by the base class destructor
}
