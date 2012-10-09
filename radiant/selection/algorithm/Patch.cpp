#include "Patch.h"

#include "selectionlib.h"

#include "patch/Patch.h"
#include "patch/PatchNode.h"
#include "ui/patch/PatchInspector.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/patch/CapDialog.h"
#include "gtkutil/dialog/MessageBox.h"

#include "selection/algorithm/Primitives.h"
#include "patch/algorithm/Prefab.h"

namespace selection
{

namespace algorithm
{

void invertPatch(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchInvert");

	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InvertMatrix(); } );
	SceneChangeNotify();
}

void redispersePatchRows(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchRedisperseRows");

	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.Redisperse(ROW); });
}

void redispersePatchCols(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchRedisperseColumns");

	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.Redisperse(COL); });
}

void transposePatch(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchTranspose");

	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.TransposeMatrix(); });

	ui::PatchInspector::Instance().queueUpdate();
}

void createPatchCaps(const std::string& shader)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		gtkutil::MessageBox::ShowError(_("Cannot create caps, no patches selected."),
			GlobalMainFrame().getTopLevelWindow());
		return;
	}

	ui::PatchCapDialog dialog;

	if (dialog.run() == ui::IDialog::RESULT_OK)
	{
		PatchPtrVector patchNodes = getSelectedPatches();

		std::for_each(patchNodes.begin(), patchNodes.end(), [&] (PatchNodePtr& patchNode)
		{
			patch::algorithm::createCaps(patchNode->getPatchInternal(), patchNode->getParent(), dialog.getSelectedCapType(), shader);
		});
	}
}

void capPatch(const cmd::ArgumentList& args)
{
	// FIXME: add support for patch cap creation
	// Patch_CapCurrent();
	UndoableCommand undo("patchCreateCaps");

	createPatchCaps(GlobalTextureBrowser().getSelectedShader());
}

void cyclePatchProjection(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchCycleUVProjectionAxis");

	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.ProjectTexture(Patch::m_CycleCapIndex); });

	Patch::m_CycleCapIndex = (Patch::m_CycleCapIndex == 0) ? 1 : (Patch::m_CycleCapIndex == 1) ? 2 : 0;

	SceneChangeNotify();
}

} // namespace

} // namespace
