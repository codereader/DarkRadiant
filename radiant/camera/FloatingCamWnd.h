#ifndef _FLOATING_CAMERA_WINDOW_H_
#define _FLOATING_CAMERA_WINDOW_H_

#include "gtkutil/window/PersistentTransientWindow.h"
#include <boost/shared_ptr.hpp>

#include "CamWnd.h"

namespace ui {

class FloatingCamWnd :
	public gtkutil::PersistentTransientWindow
{
	// The contained camera window
	CamWndPtr _camWnd;

public:
	// Pass the parent widget to this camwnd
	FloatingCamWnd(GtkWindow* parent);

	virtual ~FloatingCamWnd();
};
typedef boost::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;

} // namespace ui

#endif /* _FLOATING_CAMERA_WINDOW_H_ */
