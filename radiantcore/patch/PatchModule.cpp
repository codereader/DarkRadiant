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
#include "selectionlib.h"

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

    GlobalCommandSystem().addWithCheck(
        "PatchInsertColumnEnd",
        cmd::noArgs(selection::algorithm::insertPatchColumnsAtEnd),
        selection::pred::havePatch
    );
    GlobalCommandSystem().addWithCheck(
        "PatchInsertColumnBeginning",
        cmd::noArgs(selection::algorithm::insertPatchColumnsAtBeginning),
        selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck(
        "PatchInsertRowEnd",
        cmd::noArgs(selection::algorithm::insertPatchRowsAtEnd),
        selection::pred::havePatch
    );
    GlobalCommandSystem().addWithCheck(
        "PatchInsertRowBeginning",
        cmd::noArgs(selection::algorithm::insertPatchRowsAtBeginning),
        selection::pred::havePatch
    );

    GlobalCommandSystem().addWithCheck("PatchDeleteColumnBeginning",
                                       selection::algorithm::deletePatchColumnsFromBeginning,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("PatchDeleteColumnEnd",
                                       selection::algorithm::deletePatchColumnsFromEnd,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("PatchDeleteRowBeginning",
                                       selection::algorithm::deletePatchRowsFromBeginning,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("PatchDeleteRowEnd",
                                       selection::algorithm::deletePatchRowsFromEnd,
                                       selection::pred::havePatch);

    GlobalCommandSystem().addWithCheck("PatchAppendColumnBeginning",
                                       selection::algorithm::appendPatchColumnsAtBeginning,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("PatchAppendColumnEnd",
                                       selection::algorithm::appendPatchColumnsAtEnd,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("PatchAppendRowBeginning",
                                       selection::algorithm::appendPatchRowsAtBeginning,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("PatchAppendRowEnd",
                                       selection::algorithm::appendPatchRowsAtEnd,
                                       selection::pred::havePatch);

    GlobalCommandSystem().addWithCheck("InvertCurve", selection::algorithm::invertPatch,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("RedisperseRows",
                                       selection::algorithm::redispersePatchRows,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("RedisperseCols",
                                       selection::algorithm::redispersePatchCols,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("MatrixTranspose", selection::algorithm::transposePatch,
                                       selection::pred::havePatch);
    GlobalCommandSystem().addWithCheck("CapSelectedPatches", selection::algorithm::capPatch,
                                       selection::pred::havePatch, {cmd::ARGTYPE_STRING});
    GlobalCommandSystem().addWithCheck(
        "ThickenSelectedPatches", selection::algorithm::thickenPatches,
        selection::pred::havePatch,
        {cmd::ARGTYPE_DOUBLE, cmd::ARGTYPE_INT, cmd::ARGTYPE_INT} // thickness, create_seams, axis
    );
    GlobalCommandSystem().addWithCheck("StitchPatchTexture",
                                       cmd::noArgs(patch::algorithm::stitchTextures),
                                       [] { return selection::pred::havePatchesExact(2); });
    GlobalCommandSystem().addWithCheck("BulgePatch", patch::algorithm::bulge,
                                       selection::pred::havePatch, {cmd::ARGTYPE_DOUBLE});
    GlobalCommandSystem().addWithCheck("WeldSelectedPatches",
                                       patch::algorithm::weldSelectedPatches,
                                       [] { return selection::pred::havePatchesAtLeast(2); });
}

module::StaticModuleRegistration<PatchModule> patchModule;

}
