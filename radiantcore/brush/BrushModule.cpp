#include "BrushModule.h"

#include "i18n.h"
#include "iradiant.h"

#include "itextstream.h"
#include "ifilter.h"
#include "igame.h"
#include "ilayer.h"
#include "brush/BrushNode.h"
#include "brush/BrushClipPlane.h"
#include "brush/BrushVisit.h"
#include "gamelib.h"
#include "selectionlib.h"

#include "registry/registry.h"
#include "ipreferencesystem.h"
#include "module/StaticModule.h"
#include "messages/TextureChanged.h"

#include "selection/algorithm/Primitives.h"

// ---------------------------------------------------------------------------------------

namespace brush
{

void BrushModuleImpl::constructPreferences()
{
	// Add a page to the given group
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Primitives"));

	// Add the default texture scale preference and connect it to the according registryKey
	// Note: this should be moved somewhere else, I think
	page.appendEntry(_("Default texture scale"), "user/ui/textures/defaultTextureScale");

	// The checkbox to enable/disable the texture lock option
	page.appendCheckBox(_("Enable Texture Lock (for Brushes)"), "user/ui/brush/textureLock");
}

void BrushModuleImpl::construct()
{
	registerBrushCommands();

	Brush::m_maxWorldCoord = game::current::getValue<float>("/defaults/maxWorldCoord");
}

void BrushModuleImpl::destroy()
{
	Brush::m_maxWorldCoord = 0;
}

void BrushModuleImpl::keyChanged()
{
	_textureLockEnabled = registry::getValue<bool>(RKEY_ENABLE_TEXTURE_LOCK);
}

bool BrushModuleImpl::textureLockEnabled() const {
	return _textureLockEnabled;
}

void BrushModuleImpl::setTextureLock(bool enabled)
{
	registry::setValue(RKEY_ENABLE_TEXTURE_LOCK, enabled);
}

void BrushModuleImpl::toggleTextureLock() {
	setTextureLock(!textureLockEnabled());
}

// ------------ BrushCreator implementation --------------------------------------------

scene::INodePtr BrushModuleImpl::createBrush()
{
	scene::INodePtr node = std::make_shared<BrushNode>();

	if (GlobalMapModule().getRoot())
	{
		// All brushes are created in the active layer by default
		node->moveToLayer(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer());
	}

	return node;
}

IBrushSettings& BrushModuleImpl::getSettings()
{
	return *_settings;
}

// RegisterableModule implementation
const std::string& BrushModuleImpl::getName() const {
	static std::string _name(MODULE_BRUSHCREATOR);
	return _name;
}

const StringSet& BrushModuleImpl::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void BrushModuleImpl::initialiseModule(const IApplicationContext& ctx) {

	construct();

	_settings.reset(new BrushSettings);

	_textureLockEnabled = registry::getValue<bool>(RKEY_ENABLE_TEXTURE_LOCK);

	GlobalRegistry().signalForKey(RKEY_ENABLE_TEXTURE_LOCK).connect(
		sigc::mem_fun(this, &BrushModuleImpl::keyChanged)
	);

	// add the preference settings
	constructPreferences();

	// Set up the link to send TextureChangedMessages
	_brushFaceShaderChanged = Brush::signal_faceShaderChanged().connect(
		[] { radiant::TextureChangedMessage::Send(); });

	_faceTexDefChanged = Face::signal_texdefChanged().connect(
		[] { radiant::TextureChangedMessage::Send(); });
}

void BrushModuleImpl::shutdownModule()
{
	rMessage() << "BrushModuleImpl::shutdownModule called." << std::endl;

	_brushFaceShaderChanged.disconnect();
	_faceTexDefChanged.disconnect();

	destroy();
}

void BrushModuleImpl::registerBrushCommands()
{
	GlobalCommandSystem().addCommand("BrushMakePrefab", selection::algorithm::brushMakePrefab, { cmd::ARGTYPE_INT, cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL });
	GlobalCommandSystem().addCommand("BrushMakeSided", selection::algorithm::brushMakeSided, { cmd::ARGTYPE_INT });

	GlobalCommandSystem().addCommand("TextureNatural", selection::algorithm::naturalTexture);
    GlobalCommandSystem().addWithCheck("MakeVisportal",
                                       cmd::noArgs(selection::algorithm::makeVisportal),
                                       selection::pred::haveBrush);
    GlobalCommandSystem().addCommand("SurroundWithMonsterclip", selection::algorithm::surroundWithMonsterclip);

	GlobalCommandSystem().addCommand("ResizeSelectedBrushesToBounds", selection::algorithm::resizeSelectedBrushesToBounds,
		{ cmd::ARGTYPE_VECTOR3, cmd::ARGTYPE_VECTOR3, cmd::ARGTYPE_STRING });
}

// -------------------------------------------------------------------------------------

// Define a static BrushModule
module::StaticModuleRegistration<BrushModuleImpl> staticBrushModule;

}

// greebo: The accessor function for the brush module containing the static instance
brush::BrushModuleImpl& GlobalBrush()
{
	return *std::static_pointer_cast<brush::BrushModuleImpl>(
		module::GlobalModuleRegistry().getModule(MODULE_BRUSHCREATOR)
	);
}
