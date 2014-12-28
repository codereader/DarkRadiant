#include "FloatingCamWnd.h"

#include "i18n.h"
#include "imainframe.h"

#include <wx/sizer.h>

namespace ui
{

namespace
{
	const std::string RKEY_CAMERA_ROOT = "user/ui/camera";
	const std::string RKEY_CAMERA_WINDOW_STATE = RKEY_CAMERA_ROOT + "/window";
}

FloatingCamWnd::FloatingCamWnd(wxWindow* parent) :
	TransientWindow(_("Camera"), parent, true),
	CamWnd(this)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	GetSizer()->Add(getMainWidget(), 1, wxEXPAND);

	InitialiseWindowPosition(-1, -1, RKEY_CAMERA_WINDOW_STATE);
}

FloatingCamWnd::~FloatingCamWnd()
{
	if (IsShownOnScreen())
	{
		// Save the camera position by hiding it
		Hide();
	}
}

} // namespace
