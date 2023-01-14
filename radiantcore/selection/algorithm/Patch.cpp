#include "Patch.h"

#include "i18n.h"
#include "ipatch.h"
#include "selectionlib.h"
#include "command/ExecutionNotPossible.h"

#include "patch/Patch.h"
#include "patch/PatchNode.h"

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

	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.invertMatrix(); } );
	SceneChangeNotify();
}

void redispersePatchRows(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchRedisperseRows");

	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.redisperseRows(); });
}

void redispersePatchCols(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchRedisperseColumns");

	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.redisperseColumns(); });
}

void transposePatch(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchTranspose");

	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.transposeMatrix(); });
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
		throw cmd::ExecutionNotPossible(_("Cannot create caps, no patches selected."));
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
				capType, GlobalShaderClipboard().getShaderName());
		}
	}
	catch (const std::logic_error& ex)
	{
		rError() << ex.what() << std::endl;
	}
}

void insertPatchColumnsAtEnd()
{
	UndoableCommand undo("patchInsertColumnsAtEnd");
	// true = insert, true = columns, false = end
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(true, true, false); });
}

void insertPatchColumnsAtBeginning()
{
	UndoableCommand undo("patchInsertColumnsAtBeginning");
	// true = insert, true = columns, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(true, true, true); });
}

void insertPatchRowsAtEnd()
{
	UndoableCommand undo("patchInsertRowsAtEnd");
	// true = insert, false = rows, false = at end
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(true, false, false); });
}

void insertPatchRowsAtBeginning()
{
	UndoableCommand undo("patchInsertRowsAtBeginning");
	// true = insert, false = rows, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(true, false, true); });
}

void deletePatchColumnsFromBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteColumnsFromBeginning");
	// false = delete, true = columns, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(false, true, true); });
}

void deletePatchColumnsFromEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteColumnsFromEnd");
	// false = delete, true = columns, false = at end
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(false, true, false); });
}

void deletePatchRowsFromBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteRowsFromBeginning");
	// false = delete, false = rows, true = at beginning
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(false, false, true); });
}

void deletePatchRowsFromEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchDeleteRowsFromEnd");
	// false = delete, false = rows, false = at end
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.insertRemove(false, false, false); });
}

void appendPatchColumnsAtBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendColumnsAtBeginning");
	// true = columns, true = at the beginning
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.appendPoints(true, true); });
}

void appendPatchColumnsAtEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendColumnsAtEnd");
	// true = columns, false = at the end
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.appendPoints(true, false); });
}

void appendPatchRowsAtBeginning(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendRowsAtBeginning");
	// false = rows, true = at the beginning
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.appendPoints(false, true); });
}

void appendPatchRowsAtEnd(const cmd::ArgumentList& args)
{
	UndoableCommand undo("patchAppendRowsAtEnd");
	// false = rows, false = at the end
	GlobalSelectionSystem().foreachPatch([&] (IPatch& patch) { patch.appendPoints(false, false); });
}

/** 
 * greebo: Note: I chose to populate a list first, because otherwise the visitor
 * class would get stuck in a loop (as the newly created patches get selected,
 * and they are thickened as well, and again and again).
 */
void thickenPatches(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		throw cmd::ExecutionNotPossible(_("Cannot thicken patch. Nothing selected."));
	}

	// Check arguments
	if (args.size() != 3)
	{
		rWarning() << "Usage: ThickenSelectedPatches <thickness> <create_seams:1|0> <axis:0|1|2>" << std::endl;
		return;
	}

	float thickness = static_cast<float>(args[0].getDouble());
	bool createSeams = args[1].getInt() != 0;
	int axis = args[2].getInt();

	UndoableCommand undo("patchThicken");

	auto patches = getSelectedPatches();

	for (const PatchNodePtr& patch : patches)
	{
		patch::algorithm::thicken(patch, thickness, createSeams, axis);
	}
}

} // namespace

} // namespace
