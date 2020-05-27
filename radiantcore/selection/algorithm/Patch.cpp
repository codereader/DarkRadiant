#include "Patch.h"

#include "i18n.h"
#include "imainframe.h"
#include "selectionlib.h"

#include "patch/Patch.h"
#include "patch/PatchNode.h"
#include "ui/patch/PatchInspector.h"
#include "ui/patch/PatchThickenDialog.h"
#include "wxutil/dialog/MessageBox.h"

#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
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

patch::CapType getPatchCapTypeForString(const std::string& capTypeStr)
{
	if (capTypeStr == "bevel") return patch::CapType::Bevel;
	if (capTypeStr == "invertedbevel") return patch::CapType::InvertedBevel;
	if (capTypeStr == "endcap") return patch::CapType::EndCap;
	if (capTypeStr == "invertedendcap") return patch::CapType::InvertedEndCap;
	if (capTypeStr == "cylinder") return patch::CapType::Cylinder;

	throw std::logic_error("Invalid cap type encountered: " + capTypeStr);
}

void capPatch(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		rWarning() << "No patches selected." << std::endl;
		return;
	}

	if (args.empty())
	{
		rWarning() << "Usage: CapSelectedPatches <bevel|invertedbevel|endcap|invertedendcap|cylinder>" << std::endl;
		return;
	}

	try
	{
		// Parse the type
		auto capType = getPatchCapTypeForString(args[0].getString());

		UndoableCommand undo("patchCreateCaps");

		auto patchNodes = getSelectedPatches();

		for (const auto& patchNode : patchNodes)
		{
			patch::algorithm::createCaps(patchNode->getPatchInternal(), patchNode->getParent(), 
				capType, GlobalShaderClipboard().getSource().getShader());
		}
	}
	catch (const std::logic_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
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
		wxutil::Messagebox::ShowError(_("Cannot thicken patch. Nothing selected."));
	}
}

} // namespace

} // namespace
