#include "Clipboard.h"

#include "iselection.h"
#include "igrid.h"

#include "wxutil/clipboard.h"
#include "map/Map.h"
#include "camera/GlobalCamera.h"
#include "brush/FaceInstance.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Transformation.h"

namespace selection
{

namespace clipboard
{

void pasteToMap()
{
    GlobalSelectionSystem().setSelectedAll(false);
    std::stringstream str(wxutil::pasteFromClipboard());
    GlobalMap().importSelected(str);
}

void copy(const cmd::ArgumentList& args)
{
	if (FaceInstance::Selection().empty())
    {
        // Stream selected objects into a stringstream
        std::stringstream out;
        GlobalMap().exportSelected(out);

        // Copy the resulting string to the clipboard
        wxutil::copyToClipboard(out.str());
	}
	else
	{
		algorithm::pickShaderFromSelection(args);
	}
}

void paste(const cmd::ArgumentList& args)
{
	if (FaceInstance::Selection().empty())
    {
		UndoableCommand undo("paste");
		pasteToMap();
	}
	else
	{
		algorithm::pasteShaderToSelection(args);
	}
}

void pasteToCamera(const cmd::ArgumentList& args)
{
	ui::CamWndPtr camWnd = GlobalCamera().getActiveCamWnd();
	if (camWnd == NULL) return;

	UndoableCommand undo("pasteToCamera");
	pasteToMap();

	// Work out the delta
	Vector3 mid = algorithm::getCurrentSelectionCenter();
	Vector3 delta = camWnd->getCameraOrigin().getSnapped(GlobalGrid().getGridSize()) - mid;

	// Move to camera
	algorithm::translateSelected(delta);
}

} // namespace

} // namespace
