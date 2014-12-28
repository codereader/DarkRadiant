#pragma once

#include "wxutil/window/TransientWindow.h"
#include <memory>
#include "imainframe.h"

#include "CamWnd.h"

namespace ui
{

class FloatingCamWnd :
	public wxutil::TransientWindow,
	public CamWnd
{
public:
	// Pass the parent widget to this camwnd
	FloatingCamWnd(wxWindow* parent = GlobalMainFrame().getWxTopLevelWindow());

	virtual ~FloatingCamWnd();
};
typedef std::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;

} // namespace
