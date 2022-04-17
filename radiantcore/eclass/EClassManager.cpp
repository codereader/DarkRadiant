#include "EClassManager.h"

#include "iarchive.h"
#include "ieclasscolours.h"
#include "i18n.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "iradiant.h"
#include "ifilesystem.h"
#include "messages/ScopedLongRunningOperation.h"

#include "EntityClass.h"
#include "Doom3ModelDef.h"

#include "string/case_conv.h"
#include <functional>

#include "module/StaticModule.h"

namespace eclass {

EClassManager::EClassManager() :
    _realised(false),
    _defLoader(*this, _entityClasses, _models)
{}

sigc::signal<void>& EClassManager::defsLoadingSignal()
{
    return _defsLoadingSignal;
}

sigc::signal<void>& EClassManager::defsLoadedSignal()
{
	return _defsLoadedSignal;
}

sigc::signal<void>& EClassManager::defsReloadedSignal()
{
    return _defsReloadedSignal;
}

// Get a named entity class, creating if necessary
IEntityClassPtr EClassManager::findOrInsert(const std::string& name, bool has_brushes)
{
    ensureDefsLoaded();

    // Return an error if no name is given
    if (name.empty())
        return IEntityClassPtr();

    // Convert string to lowercase, for case-insensitive lookup
    std::string lName = string::to_lower_copy(name);

    // Find and return if exists
    EntityClass::Ptr eclass = findInternal(lName);
    if (eclass)
        return eclass;

    // Otherwise insert the new EntityClass.
    // greebo: Changed fallback behaviour when unknown entites are encountered to TRUE
    // so that brushes of unknown entites don't get lost (issue #240)
    eclass = EntityClass::createDefault(lName, true);

    // Any overrides should also apply to entityDefs that are crated on the fly
    GlobalEclassColourManager().applyColours(*eclass);

    // Try to insert the class
    return insertUnique(eclass);
}

EntityClass::Ptr EClassManager::findInternal(const std::string& name)
{
    // Find the EntityClass in the map.
    auto i = _entityClasses.find(name);

    return i != _entityClasses.end() ? i->second : EntityClass::Ptr();
}

EntityClass::Ptr EClassManager::insertUnique(const EntityClass::Ptr& eclass)
{
	// Try to insert the eclass
    auto i = _entityClasses.emplace(eclass->getName(), eclass);

    // Return the pointer to the inserted eclass
    return i.first->second;
}

void EClassManager::ensureDefsLoaded()
{
    _defLoader.ensureFinished();
}

void EClassManager::realise()
{
	if (_realised)
    {
		return; // nothing to do anymore
	}

	_realised = true;

    _defLoader.start();
}

IEntityClassPtr EClassManager::findClass(const std::string& className)
{
    ensureDefsLoaded();

	// greebo: Convert the lookup className string to lowercase first
	std::string classNameLower = string::to_lower_copy(className);

    EntityClasses::const_iterator i = _entityClasses.find(classNameLower);

    return i != _entityClasses.end() ? i->second : IEntityClassPtr();
}

void EClassManager::forEachEntityClass(EntityClassVisitor& visitor)
{
    ensureDefsLoaded();

	for (auto& [_, eclass] : _entityClasses)
	{
		visitor.visit(eclass);
	}
}

void EClassManager::unrealise()
{
    if (_realised)
	{
        // This waits for any threaded work to finish
        _defLoader.reset();
       	_realised = false;
    }
}

IModelDefPtr EClassManager::findModel(const std::string& name)
{
    ensureDefsLoaded();

	auto found = _models.find(name);
	return found != _models.end() ? found->second : Doom3ModelDef::Ptr();
}

void EClassManager::forEachModelDef(ModelDefVisitor& visitor)
{
    forEachModelDef([&](const IModelDefPtr& def)
    {
        visitor.visit(def);
    });
}

void EClassManager::forEachModelDef(const std::function<void(const IModelDefPtr&)>& functor)
{
    ensureDefsLoaded();

    for (const auto& pair : _models)
    {
        functor(pair.second);
    }
}

void EClassManager::reloadDefs()
{
	// greebo: Leave all current entityclasses as they are, just invoke the
	// FileLoader again. It will parse the files again, and look up
	// the eclass names in the existing map. If found, the eclass
	// will be asked to clear itself and re-parse from the tokens.
	// This is to assure that any IEntityClassPtrs remain intact during
	// the process, only the class contents change.
    _defLoader.parseSynchronously();

    // On top of the "loaded" signal, emit the "reloaded" signal
    _defsReloadedSignal.emit();
}

// RegisterableModule implementation
const std::string& EClassManager::getName() const
{
	static std::string _name(MODULE_ECLASSMANAGER);
	return _name;
}

const StringSet& EClassManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_VIRTUALFILESYSTEM,
        MODULE_XMLREGISTRY,
        MODULE_COMMANDSYSTEM,
        MODULE_ECLASS_COLOUR_MANAGER
    };

	return _dependencies;
}

void EClassManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalFileSystem().addObserver(*this);

    if (GlobalFileSystem().isInitialised())
    {
        realise();
    }

	GlobalCommandSystem().addCommand("ReloadDefs", std::bind(&EClassManager::reloadDefsCmd, this, std::placeholders::_1));

    _eclassColoursChanged = GlobalEclassColourManager().sig_overrideColourChanged().connect(
        sigc::mem_fun(this, &EClassManager::onEclassOverrideColourChanged));
}

void EClassManager::shutdownModule()
{
	rMessage() << "EntityClassDoom3::shutdownModule called." << std::endl;

    _eclassColoursChanged.disconnect();

	GlobalFileSystem().removeObserver(*this);

	// Unrealise ourselves and wait for threads to finish
	unrealise();

	// Don't notify anyone anymore
	_defsReloadedSignal.clear();
    _defsLoadedSignal.clear();
    _defsLoadingSignal.clear();

	// Clear member structures
	_entityClasses.clear();
	_models.clear();
}

void EClassManager::onEclassOverrideColourChanged(const std::string& eclass, bool overrideRemoved)
{
    // An override colour in the IColourManager instance has changed
    // Do we have an affected eclass with that name?
    auto foundEclass = _entityClasses.find(eclass);

    if (foundEclass == _entityClasses.end())
    {
        return;
    }

    // If the override was removed, we just reset the colour
    // We perform this switch to avoid firing the eclass changed signal twice
    if (overrideRemoved)
    {
        foundEclass->second->resetColour();
    }
    else
    {
        GlobalEclassColourManager().applyColours(*foundEclass->second);
    }
}

// This takes care of relading the entityDefs and refreshing the scenegraph
void EClassManager::reloadDefsCmd(const cmd::ArgumentList& args)
{
	radiant::ScopedLongRunningOperation operation(_("Reloading Defs"));

    reloadDefs();
}

// Gets called on VFS initialise
void EClassManager::onFileSystemInitialise()
{
	realise();
}

// Gets called on VFS shutdown
void EClassManager::onFileSystemShutdown()
{
	unrealise();
}

// Static module instance
module::StaticModuleRegistration<EClassManager> eclassModule;

} // namespace eclass
