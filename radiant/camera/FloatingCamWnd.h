#pragma once

#include "wxutil/window/TransientWindow.h"
#include <boost/shared_ptr.hpp>
#include "imainframe.h"

#include "CamWnd.h"

class FloatingCamWnd :
	public wxutil::TransientWindow,
	public CamWnd
{
public:
	// Pass the parent widget to this camwnd
	FloatingCamWnd(wxWindow* parent = GlobalMainFrame().getWxTopLevelWindow());

	virtual ~FloatingCamWnd();
};
typedef boost::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;
