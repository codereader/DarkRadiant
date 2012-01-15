#pragma once

#include "gtkutil/window/PersistentTransientWindow.h"
#include <boost/shared_ptr.hpp>

#include "CamWnd.h"

class FloatingCamWnd :
	public gtkutil::PersistentTransientWindow,
	public CamWnd
{
public:
	// Pass the parent widget to this camwnd
	FloatingCamWnd(const Glib::RefPtr<Gtk::Window>& parent);

	virtual ~FloatingCamWnd();
};
typedef boost::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;
