#include "Clipboard.h"

#include "i18n.h"
#include "iselection.h"
#include "igrid.h"
#include "icameraview.h"
#include "imapformat.h"
#include "iclipboard.h"
#include "ishaderclipboard.h"
#include "string/trim.h"

#include "map/Map.h"
#include "brush/FaceInstance.h"
#include "map/algorithm/Import.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Transformation.h"
#include "command/ExecutionFailure.h"
#include "command/ExecutionNotPossible.h"
#include "messages/MapOperationMessage.h"

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

void copySelectedMapElementsToClipboard()
{
    // When exporting to the system clipboard, use the portable format
    auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

    // Stream selected objects into a stringstream
    std::stringstream out;
    GlobalMap().exportSelected(out, format);

    // Copy the resulting string to the clipboard
    GlobalClipboard().setString(out.str());
}

void copy(const cmd::ArgumentList& args)
{
	if (FaceInstance::Selection().empty())
    {
		if (!module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD))
		{
			throw cmd::ExecutionNotPossible(_("No clipboard module attached, cannot perform this action."));
		}

        if (GlobalSelectionSystem().countSelected() == 0)
        {
            map::OperationMessage::Send(_("Nothing copied"));
            return;
        }

        copySelectedMapElementsToClipboard();
        map::OperationMessage::Send(_("Selection copied to Clipboard"));
	}
	else
	{
		algorithm::pickShaderFromSelection(args);
        map::OperationMessage::Send(_("Face Texture copied to Clipboard"));
	}
}

void cut(const cmd::ArgumentList& args)
{
    if (!module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD))
    {
        throw cmd::ExecutionNotPossible(_("No clipboard module attached, cannot perform this action."));
    }

    if (!FaceInstance::Selection().empty())
    {
        throw cmd::ExecutionNotPossible(_("Cannot cut selected Faces."));
    }

    if (GlobalSelectionSystem().countSelected() == 0)
    {
        map::OperationMessage::Send(_("Nothing to cut"));
        return;
    }

    UndoableCommand cmd("Cut Selection");

    copySelectedMapElementsToClipboard();
    algorithm::deleteSelection();
}

std::string getMaterialNameFromClipboard()
{
    if (!module::GlobalModuleRegistry().moduleExists(MODULE_CLIPBOARD))
    {
        return std::string();
    }

    auto candidate = GlobalClipboard().getString();
    string::trim(candidate);

    // If we get a single line, check if the contents point to a material we know
    if (!candidate.empty() && candidate.find('\n') == std::string::npos &&
        GlobalMaterialManager().materialExists(candidate))
    {
        return candidate;
    }
    
    return std::string();
}

void paste(const cmd::ArgumentList& args)
{
	if (FaceInstance::Selection().empty())
    {
        auto clipboardMaterial = getMaterialNameFromClipboard();

        if (!clipboardMaterial.empty())
        {
            UndoableCommand undo("pasteMaterialFromClipboard");

            // Activate the material name in the shader clipboard, but don't overwrite
            // anything there if the material is already matching to not overwrite Face/Patch information
            if (GlobalShaderClipboard().getShaderName() != clipboardMaterial)
            {
                GlobalShaderClipboard().setSourceShader(clipboardMaterial);
            }

            algorithm::pasteShaderToSelection(args);
            return;
        }

        // Try to parse the map and apply it
        UndoableCommand undo("Paste");
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
		auto& camWnd = GlobalCameraManager().getActiveView();

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
