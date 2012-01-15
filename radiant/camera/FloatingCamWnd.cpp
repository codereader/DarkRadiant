#include "FloatingCamWnd.h"

#include "i18n.h"
#include "gtkutil/FramedWidget.h"

#include "GlobalCamera.h"

FloatingCamWnd::FloatingCamWnd(const Glib::RefPtr<Gtk::Window>& parent) :
	PersistentTransientWindow(_("Camera"), parent, true)
{
	CamWnd::setContainer(getRefPtr());

	add(*Gtk::manage(new gtkutil::FramedWidget(*CamWnd::getWidget())));

	set_type_hint(Gdk::WINDOW_TYPE_HINT_NORMAL);

#ifdef WIN32
	// This is to fix camviews from going grey in Windows, due to some GTK/GtkGLExt bug
	CamWnd::connectWindowStateEvent(*this);
#endif
}

FloatingCamWnd::~FloatingCamWnd()
{
#ifdef WIN32
	CamWnd::disconnectWindowStateEvent();
#endif

	// Disconnect the camera from its parent container
	CamWnd::setContainer(Glib::RefPtr<Gtk::Window>());

	// GtkWindow destruction is handled by the base class destructor
}
