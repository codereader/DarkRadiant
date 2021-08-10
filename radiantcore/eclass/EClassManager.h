#pragma once

#include <sigc++/connection.h>
#include "ieclass.h"
#include "icommandsystem.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "ThreadedDefLoader.h"

#include "EntityClass.h"
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
 * This class is the master loader for the entity classes. It ensures that the
 * loadFile() function is called for every .def file in the def/ directory,
 * which in turn kicks off the parse process (including the resolution of
 * inheritance).
 *
 */
class EClassManager :
    public IEntityClassManager,
    public vfs::VirtualFileSystem::Observer
{
    // Whether the entity classes have been realised
    bool _realised;

    // Map of named entity classes
    typedef std::map<std::string, EntityClass::Ptr> EntityClasses;
    EntityClasses _entityClasses;

    typedef std::map<std::string, Doom3ModelDef::Ptr> Models;
    Models _models;

    // The worker thread loading the eclasses will be managed by this
    util::ThreadedDefLoader<void> _defLoader;

	// A unique parse pass identifier, used to check when existing
	// definitions have been parsed
	std::size_t _curParseStamp;

    sigc::signal<void> _defsLoadingSignal;
    sigc::signal<void> _defsLoadedSignal;
    sigc::signal<void> _defsReloadedSignal;

    sigc::connection _eclassColoursChanged;

public:
    // Constructor
	EClassManager();

    // IEntityClassManager implementation
    sigc::signal<void> defsLoadingSignal() const override;
    sigc::signal<void> defsLoadedSignal() const override;
    sigc::signal<void> defsReloadedSignal() const override;
    virtual IEntityClassPtr findOrInsert(const std::string& name,
                                         bool has_brushes) override;
    IEntityClassPtr findClass(const std::string& className) override;
    virtual void forEachEntityClass(EntityClassVisitor& visitor) override;
    void realise() override;
    void unrealise() override;

    // VFS::Observer implementation
    virtual void onFileSystemInitialise() override;
    virtual void onFileSystemShutdown() override;

    // Find the modeldef with the given name
    virtual IModelDefPtr findModel(const std::string& name) override;
    virtual void forEachModelDef(ModelDefVisitor& visitor) override;
    virtual void forEachModelDef(const std::function<void(const IModelDefPtr&)>& functor) override;

	// Reloads all entityDefs/modelDefs
    void reloadDefs() override;

    // RegisterableModule implementation
	const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
	// Method loading the DEF files
    void parseFile(const vfs::FileInfo& fileInfo);

    // Since loading is happening in a worker thread, we need to ensure
    // that it's done loading before accessing any defs or models.
    void ensureDefsLoaded();

    // Worker function usually done in a separate thread
    void loadDefAndResolveInheritance();

    // Invokes the change notifications that have been buffered during parsing
    void onDefLoadingCompleted();

    // Applies possible colour overrides to all affected eclasses
    void applyColours();

	// Tries to insert the given eclass, not overwriting existing ones
	// In either case, the eclass in the map is returned
	EntityClass::Ptr insertUnique(const EntityClass::Ptr& eclass);
    EntityClass::Ptr findInternal(const std::string& name);

	// Parses the given inputstream for DEFs.
	void parse(TextInputStream& inStr, const vfs::FileInfo& fileInfo, const std::string& modDir);

	// Recursively resolves the inheritance of the model defs
	void resolveModelInheritance(const std::string& name, const Doom3ModelDef::Ptr& model);

	void parseDefFiles();
	void resolveInheritance();

	void reloadDefsCmd(const cmd::ArgumentList& args);

    void onEclassOverrideColourChanged(const std::string& eclass, bool overrideRemoved);
};
typedef std::shared_ptr<EClassManager> EClassManagerPtr;

} // namespace eclass
