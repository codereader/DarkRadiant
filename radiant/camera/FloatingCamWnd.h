#ifndef _FLOATING_CAMERA_WINDOW_H_
#define _FLOATING_CAMERA_WINDOW_H_

#include "gtkutil/window/PersistentTransientWindow.h"
#include <boost/shared_ptr.hpp>

#include "CamWnd.h"

class FloatingCamWnd :
	public gtkutil::PersistentTransientWindow,
	public CamWnd
{
public:
	// Pass the parent widget to this camwnd
	FloatingCamWnd(GtkWindow* parent);

	virtual ~FloatingCamWnd();
};
typedef boost::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;

#endif /* _FLOATING_CAMERA_WINDOW_H_ */
