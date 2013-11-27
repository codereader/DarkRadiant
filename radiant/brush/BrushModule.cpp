#include "BrushModule.h"

#include "i18n.h"
#include "iradiant.h"

#include "itextstream.h"
#include "ifilter.h"
#include "igame.h"
#include "ilayer.h"
#include "ieventmanager.h"
#include "brush/BrushNode.h"
#include "brush/BrushClipPlane.h"
#include "brush/BrushVisit.h"
#include "gamelib.h"

#include "registry/registry.h"
#include "ipreferencesystem.h"
#include "modulesystem/StaticModule.h"

#include "selection/algorithm/Primitives.h"

// ---------------------------------------------------------------------------------------

void BrushModuleImpl::constructPreferences()
{
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Primitives"));

	// Add the default texture scale preference and connect it to the according registryKey
	// Note: this should be moved somewhere else, I think
	page->appendEntry(_("Default texture scale"), "user/ui/textures/defaultTextureScale");

	// The checkbox to enable/disable the texture lock option
	page->appendCheckBox("", _("Enable Texture Lock (for Brushes)"), "user/ui/brush/textureLock");
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
	scene::INodePtr node(new BrushNode);

	// Move it to the active layer
	node->moveToLayer(GlobalLayerSystem().getActiveLayer());

	return node;
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
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_UNDOSYSTEM);
	}

	return _dependencies;
}

void BrushModuleImpl::initialiseModule(const ApplicationContext& ctx) {
	rMessage() << "BrushModuleImpl::initialiseModule called." << std::endl;

	construct();

	_textureLockEnabled = registry::getValue<bool>(RKEY_ENABLE_TEXTURE_LOCK);

	GlobalRegistry().signalForKey(RKEY_ENABLE_TEXTURE_LOCK).connect(
        sigc::mem_fun(this, &BrushModuleImpl::keyChanged)
    );

	// add the preference settings
	constructPreferences();
}

void BrushModuleImpl::shutdownModule() {
	rMessage() << "BrushModuleImpl::shutdownModule called." << std::endl;
	destroy();
}

void BrushModuleImpl::registerBrushCommands()
{
	GlobalEventManager().addRegistryToggle("TogTexLock", RKEY_ENABLE_TEXTURE_LOCK);

	GlobalCommandSystem().addCommand("BrushMakePrefab", selection::algorithm::brushMakePrefab, cmd::ARGTYPE_INT);

	GlobalEventManager().addCommand("BrushCuboid", "BrushCuboid");
	GlobalEventManager().addCommand("BrushPrism", "BrushPrism");
	GlobalEventManager().addCommand("BrushCone", "BrushCone");
	GlobalEventManager().addCommand("BrushSphere", "BrushSphere");

	GlobalCommandSystem().addCommand("BrushMakeSided", selection::algorithm::brushMakeSided, cmd::ARGTYPE_INT);

	// Link the Events to the corresponding statements
	GlobalEventManager().addCommand("Brush3Sided", "Brush3Sided");
	GlobalEventManager().addCommand("Brush4Sided", "Brush4Sided");
	GlobalEventManager().addCommand("Brush5Sided", "Brush5Sided");
	GlobalEventManager().addCommand("Brush6Sided", "Brush6Sided");
	GlobalEventManager().addCommand("Brush7Sided", "Brush7Sided");
	GlobalEventManager().addCommand("Brush8Sided", "Brush8Sided");
	GlobalEventManager().addCommand("Brush9Sided", "Brush9Sided");

	GlobalCommandSystem().addCommand("TextureNatural", selection::algorithm::naturalTexture);
	GlobalCommandSystem().addCommand("MakeVisportal", selection::algorithm::makeVisportal);
	GlobalCommandSystem().addCommand("SurroundWithMonsterclip", selection::algorithm::surroundWithMonsterclip);
	GlobalEventManager().addCommand("TextureNatural", "TextureNatural");
	GlobalEventManager().addCommand("MakeVisportal", "MakeVisportal");
	GlobalEventManager().addCommand("SurroundWithMonsterclip", "SurroundWithMonsterclip");
}

// -------------------------------------------------------------------------------------

// Define a static BrushModule
module::StaticModule<BrushModuleImpl> staticBrushModule;

// greebo: The accessor function for the brush module containing the static instance
BrushModuleImpl& GlobalBrush()
{
	return *staticBrushModule.getModule().get();
}
