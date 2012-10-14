#include "Patch.h"

#include "i18n.h"
#include "imainframe.h"
#include "selectionlib.h"

#include "patch/Patch.h"
#include "patch/PatchNode.h"
#include "ui/patch/PatchInspector.h"
#include "ui/patch/PatchThickenDialog.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/patch/CapDialog.h"
#include "gtkutil/dialog/MessageBox.h"

#include "selection/algorithm/Primitives.h"
#include "patch/algorithm/Prefab.h"
#include "patch/algorithm/General.h"

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

void insertPatchColumnsAtEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchInsertColumnsAtEnd");
	// true = insert, true = columns, false = end
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(true, true, false); });
}

void insertPatchColumnsAtBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchInsertColumnsAtBeginning");
	// true = insert, true = columns, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(true, true, true); });
}

void insertPatchRowsAtEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchInsertRowsAtEnd");
	// true = insert, false = rows, false = at end
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(true, false, false); });
}

void insertPatchRowsAtBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchInsertRowsAtBeginning");
	// true = insert, false = rows, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(true, false, true); });
}

void deletePatchColumnsFromBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteColumnsFromBeginning");
	// false = delete, true = columns, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(false, true, true); });
}

void deletePatchColumnsFromEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteColumnsFromEnd");
	// false = delete, true = columns, false = at end
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(false, true, false); });
}

void deletePatchRowsFromBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteRowsFromBeginning");
	// false = delete, false = rows, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(false, false, true); });
}

void deletePatchRowsFromEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteRowsFromEnd");
	// false = delete, false = rows, false = at end
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.InsertRemove(false, false, false); });
}

void appendPatchColumnsAtBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendColumnsAtBeginning");
	// true = columns, true = at the beginning
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.appendPoints(true, true); });
}

void appendPatchColumnsAtEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendColumnsAtEnd");
	// true = columns, false = at the end
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.appendPoints(true, false); });
}

void appendPatchRowsAtBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendRowsAtBeginning");
	// false = rows, true = at the beginning
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.appendPoints(false, true); });
}

void appendPatchRowsAtEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendRowsAtEnd");
	// false = rows, false = at the end
	GlobalSelectionSystem().foreachPatch([&] (Patch& patch) { patch.appendPoints(false, false); });
}

/** 
 * greebo: Note: I chose to populate a list first, because otherwise the visitor
 * class would get stuck in a loop (as the newly created patches get selected,
 * and they are thickened as well, and again and again).
 */
void thickenPatches(const cmd::ArgumentList& args)
{
	// Get all the selected patches
	PatchPtrVector patchList = getSelectedPatches();

	if (!patchList.empty())
	{
		UndoableCommand undo("patchThicken");

		ui::PatchThickenDialog dialog;

		if (dialog.run() == ui::IDialog::RESULT_OK)
		{
			// Go through the list and thicken all the found ones
			for (std::size_t i = 0; i < patchList.size(); i++)
			{
				patch::algorithm::thicken(patchList[i], dialog.getThickness(), dialog.getCeateSeams(), dialog.getAxis());
			}
		}
	}
	else
	{
		gtkutil::MessageBox::ShowError(_("Cannot thicken patch. Nothing selected."),
							 GlobalMainFrame().getTopLevelWindow());
	}
}

} // namespace

} // namespace
