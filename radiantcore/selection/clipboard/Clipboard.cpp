#include "Clipboard.h"

#include "iselection.h"
#include "igrid.h"
#include "icamera.h"
#include "imapformat.h"
#include "iclipboard.h"

#include "map/Map.h"
#include "brush/FaceInstance.h"
#include "map/algorithm/Import.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Transformation.h"
#include "command/ExecutionNotPossible.h"

namespace selection
{

namespace clipboard
{

void pasteToMap()
{
	if (!module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD))
	{
		throw cmd::ExecutionNotPossible(_("No clipboard module attached, cannot perform this action."));
	}

    std::stringstream stream(GlobalClipboard().getString());
	map::algorithm::importFromStream(stream);
}

void copy(const cmd::ArgumentList& args)
{
	if (FaceInstance::Selection().empty())
    {
		if (!module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD))
		{
			throw cmd::ExecutionNotPossible(_("No clipboard module attached, cannot perform this action."));
		}

		// When exporting to the system clipboard, use the portable format
		auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

        // Stream selected objects into a stringstream
        std::stringstream out;
        GlobalMap().exportSelected(out, format);

        // Copy the resulting string to the clipboard
		GlobalClipboard().setString(out.str());
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
	try
	{
		auto& camWnd = GlobalCameraView().getActiveView();

		UndoableCommand undo("pasteToCamera");
		pasteToMap();

		// Work out the delta
		Vector3 mid = algorithm::getCurrentSelectionCenter();
		Vector3 delta = camWnd.getCameraOrigin().getSnapped(GlobalGrid().getGridSize()) - mid;

		// Move to camera
		algorithm::translateSelected(delta);
	}
	catch (const std::runtime_error& ex)
	{
		rError() << "Cannot paste to camera: " << ex.what() << std::endl;
	}
}

} // namespace

} // namespace
