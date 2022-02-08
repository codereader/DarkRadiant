#include "PatchModule.h"

#include "ifilter.h"
#include "ilayer.h"
#include "imap.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include "i18n.h"

#include "PatchNode.h"

#include "patch/algorithm/Prefab.h"
#include "patch/algorithm/General.h"
#include "selection/algorithm/Patch.h"

#include "module/StaticModule.h"
#include "messages/TextureChanged.h"

namespace patch
{

namespace
{
	const char* const RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
}

scene::INodePtr PatchModule::createPatch(PatchDefType type)
{
	scene::INodePtr node = std::make_shared<PatchNode>(type);

	if (GlobalMapModule().getRoot())
	{
		// All patches are created in the active layer by default
		node->moveToLayer(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer());
	}

	return node;
}

IPatchSettings& PatchModule::getSettings()
{
	return *_settings;
}

const std::string& PatchModule::getName() const
{
	static std::string _name(MODULE_PATCH);
	return _name;
}

const StringSet& PatchModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_RENDERSYSTEM);
	}

	return _dependencies;
}

void PatchModule::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_settings.reset(new PatchSettings);

	registerPatchCommands();

	// Construct and Register the patch-related preferences
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Patch"));
	page.appendEntry(_("Patch Subdivide Threshold"), RKEY_PATCH_SUBDIVIDE_THRESHOLD);

	_patchTextureChanged = Patch::signal_patchTextureChanged().connect(
		[] { radiant::TextureChangedMessage::Send(); });
}

void PatchModule::shutdownModule()
{
	_patchTextureChanged.disconnect();
}

void PatchModule::registerPatchCommands()
{
	// First connect the commands to the code
	GlobalCommandSystem().addCommand("CreatePatchPrefab", patch::algorithm::createPrefab, { cmd::ARGTYPE_STRING });

	// Two optional integer arguments
	GlobalCommandSystem().addCommand("CreateSimplePatchMesh", patch::algorithm::createSimplePatch,
		{ cmd::ARGTYPE_INT, cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL }); // dimX, dimY, removeSelectedBrush

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
	GlobalCommandSystem().addCommand("CapSelectedPatches", selection::algorithm::capPatch, { cmd::ARGTYPE_STRING });
	GlobalCommandSystem().addCommand("ThickenSelectedPatches", selection::algorithm::thickenPatches,
		{ cmd::ARGTYPE_DOUBLE, cmd::ARGTYPE_INT, cmd::ARGTYPE_INT }); // thickness, create_seams, axis
	GlobalCommandSystem().addCommand("StitchPatchTexture", patch::algorithm::stitchTextures);
	GlobalCommandSystem().addCommand("BulgePatch", patch::algorithm::bulge, { cmd::ARGTYPE_DOUBLE });
	GlobalCommandSystem().addCommand("WeldSelectedPatches", patch::algorithm::weldSelectedPatches);
}

module::StaticModuleRegistration<PatchModule> patchModule;

}
