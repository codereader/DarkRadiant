#include "PatchCreators.h"

#include "ifilter.h"
#include "ilayer.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include "i18n.h"

#include "PatchNode.h"

#include "patch/algorithm/Prefab.h"
#include "patch/algorithm/General.h"
#include "selection/algorithm/Patch.h"

namespace
{
	const char* const RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
}

scene::INodePtr Doom3PatchCreator::createPatch()
{
	// Note the true as function argument:
	// this means that patchDef3 = true in the PatchNode constructor.
	scene::INodePtr node(new PatchNode(true));

	// Determine the layer patches should be created in
	int layer = GlobalLayerSystem().getActiveLayer();

	// Move it to the first visible layer
	node->moveToLayer(layer);
		
	return node;
}

// RegisterableModule implementation
const std::string& Doom3PatchCreator::getName() const
{
	static std::string _name(MODULE_PATCH + DEF3);
	return _name;
}

const StringSet& Doom3PatchCreator::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void Doom3PatchCreator::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	registerPatchCommands();

	// Construct and Register the patch-related preferences
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Patch"));
	page->appendEntry(_("Patch Subdivide Threshold"), RKEY_PATCH_SUBDIVIDE_THRESHOLD);
}

void Doom3PatchCreator::registerPatchCommands()
{
	// First connect the commands to the code
	GlobalCommandSystem().addCommand("CreatePatchPrefab", patch::algorithm::createPrefab, cmd::ARGTYPE_STRING);

	// Two optional integer arguments
	GlobalCommandSystem().addCommand("SimplePatchMesh", patch::algorithm::createSimplePatch,
		cmd::Signature(cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL));

	GlobalCommandSystem().addCommand("PatchInsertColumnEnd", selection::algorithm::insertPatchColumnsAtEnd);
	GlobalCommandSystem().addCommand("PatchInsertColumnBeginning", selection::algorithm::insertPatchColumnsAtBeginning);
	GlobalCommandSystem().addCommand("PatchInsertRowEnd", selection::algorithm::insertPatchRowsAtEnd);
	GlobalCommandSystem().addCommand("PatchInsertRowBeginning", selection::algorithm::insertPatchRowsAtBeginning);

	GlobalCommandSystem().addCommand("PatchDeleteColumnBeginning", selection::algorithm::deletePatchColumnsFromBeginning);
	GlobalCommandSystem().addCommand("PatchDeleteColumnEnd", selection::algorithm::deletePatchColumnsFromEnd);
	GlobalCommandSystem().addCommand("PatchDeleteRowBeginning", selection::algorithm::deletePatchRowsFromBeginning);
	GlobalCommandSystem().addCommand("PatchDeleteRowEnd", selection::algorithm::deletePatchRowsFromEnd);

	GlobalCommandSystem().addCommand("PatchAppendColumnBeginning", selection::algorithm::appendPatchColumnsAtBeginning);
	GlobalCommandSystem().addCommand("PatchAppendColumnEnd", selection::algorithm::appendPatchColumnsAtEnd);
	GlobalCommandSystem().addCommand("PatchAppendRowBeginning", selection::algorithm::appendPatchRowsAtBeginning);
	GlobalCommandSystem().addCommand("PatchAppendRowEnd", selection::algorithm::appendPatchRowsAtEnd);

	GlobalCommandSystem().addCommand("InvertCurve", selection::algorithm::invertPatch);
	GlobalCommandSystem().addCommand("RedisperseRows", selection::algorithm::redispersePatchRows);
	GlobalCommandSystem().addCommand("RedisperseCols", selection::algorithm::redispersePatchCols);
	GlobalCommandSystem().addCommand("MatrixTranspose", selection::algorithm::transposePatch);
	GlobalCommandSystem().addCommand("CapCurrentCurve", selection::algorithm::capPatch);
	GlobalCommandSystem().addCommand("CycleCapTexturePatch", selection::algorithm::cyclePatchProjection);
	GlobalCommandSystem().addCommand("ThickenPatch", selection::algorithm::thickenPatches);
	GlobalCommandSystem().addCommand("StitchPatchTexture", patch::algorithm::stitchTextures);
	GlobalCommandSystem().addCommand("BulgePatch", patch::algorithm::bulge);

	// Then, connect the Events to the commands
	GlobalEventManager().addCommand("PatchCylinder", "PatchCylinder");
	GlobalEventManager().addCommand("PatchDenseCylinder", "PatchDenseCylinder");
	GlobalEventManager().addCommand("PatchVeryDenseCylinder", "PatchVeryDenseCylinder");
	GlobalEventManager().addCommand("PatchSquareCylinder", "PatchSquareCylinder");
	GlobalEventManager().addCommand("PatchEndCap", "PatchEndCap");
	GlobalEventManager().addCommand("PatchBevel", "PatchBevel");
	GlobalEventManager().addCommand("PatchCone", "PatchCone");
	GlobalEventManager().addCommand("PatchSphere", "PatchSphere");
	GlobalEventManager().addCommand("SimplePatchMesh", "SimplePatchMesh");

	GlobalEventManager().addCommand("PatchInsertColumnEnd", "PatchInsertColumnEnd");
	GlobalEventManager().addCommand("PatchInsertColumnBeginning", "PatchInsertColumnBeginning");
	GlobalEventManager().addCommand("PatchInsertRowEnd", "PatchInsertRowEnd");
	GlobalEventManager().addCommand("PatchInsertRowBeginning", "PatchInsertRowBeginning");

	GlobalEventManager().addCommand("PatchDeleteColumnBeginning", "PatchDeleteColumnBeginning");
	GlobalEventManager().addCommand("PatchDeleteColumnEnd", "PatchDeleteColumnEnd");
	GlobalEventManager().addCommand("PatchDeleteRowBeginning", "PatchDeleteRowBeginning");
	GlobalEventManager().addCommand("PatchDeleteRowEnd", "PatchDeleteRowEnd");

	GlobalEventManager().addCommand("PatchAppendColumnBeginning", "PatchAppendColumnBeginning");
	GlobalEventManager().addCommand("PatchAppendColumnEnd", "PatchAppendColumnEnd");
	GlobalEventManager().addCommand("PatchAppendRowBeginning", "PatchAppendRowBeginning");
	GlobalEventManager().addCommand("PatchAppendRowEnd", "PatchAppendRowEnd");

	GlobalEventManager().addCommand("InvertCurve", "InvertCurve");
	GlobalEventManager().addCommand("RedisperseRows", "RedisperseRows");
	GlobalEventManager().addCommand("RedisperseCols", "RedisperseCols");
	GlobalEventManager().addCommand("MatrixTranspose", "MatrixTranspose");
	GlobalEventManager().addCommand("CapCurrentCurve", "CapCurrentCurve");
	GlobalEventManager().addCommand("CycleCapTexturePatch", "CycleCapTexturePatch");
	GlobalEventManager().addCommand("ThickenPatch", "ThickenPatch");
	GlobalEventManager().addCommand("StitchPatchTexture", "StitchPatchTexture");
	GlobalEventManager().addCommand("BulgePatch", "BulgePatch");
}

// --------------------------------------------

scene::INodePtr Doom3PatchDef2Creator::createPatch()
{
	// The PatchNodeDoom3 constructor takes false == patchDef2
	scene::INodePtr node(new PatchNode(false));

	// Determine the layer patches should be created in
	int layer = GlobalLayerSystem().getActiveLayer();

	// Move it to the first visible layer
	node->moveToLayer(layer);
		
	return node;
}

// RegisterableModule implementation
const std::string& Doom3PatchDef2Creator::getName() const
{
	static std::string _name(MODULE_PATCH + DEF2);
	return _name;
}

const StringSet& Doom3PatchDef2Creator::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void Doom3PatchDef2Creator::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}
