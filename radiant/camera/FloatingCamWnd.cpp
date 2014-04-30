#include "FloatingCamWnd.h"

#include "i18n.h"
#include "imainframe.h"

FloatingCamWnd::FloatingCamWnd(wxWindow* parent) :
	TransientWindow(_("Camera"), parent, true),
	CamWnd(this)
{}

FloatingCamWnd::~FloatingCamWnd()
{}
