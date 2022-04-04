#pragma once

#include <sigc++/connection.h>

#include "ieclass.h"
#include "icommandsystem.h"
#include "ifilesystem.h"
#include "itextstream.h"

#include "EntityClass.h"
#include "EClassParser.h"
#include "Doom3ModelDef.h"

namespace eclass
{

/**
 * \namespace eclass
 * Implementing classes for the Entity Class manager module.
 * \ingroup eclass
 */

/**
 * EClassManager - master entity loader
 *
 * This class is the loader for the entity classes. It ensures that the
 * loadFile() function is called for every .def file in the def/ directory,
 * which in turn kicks off the parse process (including the resolution of
 * inheritance).
 *
 */
class EClassManager final :
    public IEntityClassManager,
    public vfs::VirtualFileSystem::Observer
{
private:
    // Whether the entity classes have been realised
    bool _realised;

    // Map of named entity classes
    typedef std::map<std::string, EntityClass::Ptr> EntityClasses;
    EntityClasses _entityClasses;

    typedef std::map<std::string, Doom3ModelDef::Ptr> Models;
    Models _models;

    // The worker thread loading the eclasses will be managed by this
    EClassParser _defLoader;

    sigc::signal<void> _defsLoadingSignal;
    sigc::signal<void> _defsLoadedSignal;
    sigc::signal<void> _defsReloadedSignal;

    sigc::connection _eclassColoursChanged;

public:
	EClassManager();

    // IEntityClassManager implementation
    sigc::signal<void>& defsLoadingSignal() override;
    sigc::signal<void>& defsLoadedSignal() override;
    sigc::signal<void>& defsReloadedSignal() override;
    IEntityClassPtr findOrInsert(const std::string& name, bool has_brushes) override;
    IEntityClassPtr findClass(const std::string& className) override;
    void forEachEntityClass(EntityClassVisitor& visitor) override;
    void realise() override;
    void unrealise() override;

    // VFS::Observer implementation
    void onFileSystemInitialise() override;
    void onFileSystemShutdown() override;

    // Find the modeldef with the given name
    IModelDefPtr findModel(const std::string& name) override;
    void forEachModelDef(ModelDefVisitor& visitor) override;
    void forEachModelDef(const std::function<void(const IModelDefPtr&)>& functor) override;

	// Reloads all entityDefs/modelDefs
    void reloadDefs() override;

    // RegisterableModule implementation
	const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    // Since loading is happening in a worker thread, we need to ensure
    // that it's done loading before accessing any defs or models.
    void ensureDefsLoaded();

	// Tries to insert the given eclass, not overwriting existing ones
	// In either case, the eclass in the map is returned
	EntityClass::Ptr insertUnique(const EntityClass::Ptr& eclass);
    EntityClass::Ptr findInternal(const std::string& name);

	void reloadDefsCmd(const cmd::ArgumentList& args);

    void onEclassOverrideColourChanged(const std::string& eclass, bool overrideRemoved);
};

} // namespace
