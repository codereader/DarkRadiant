#pragma once

#include <future>
#include "ieclass.h"
#include "icommandsystem.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "moduleobservers.h"

#include "Doom3EntityClass.h"
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
 * It also accomodates ModuleObservers, presumably to be notified when the
 * dependency modules are realised. This one depends on the VFS.
 */
class EClassManager :
    public IEntityClassManager,
    public VirtualFileSystem::Observer
{
    // Whether the entity classes have been realised
    bool _realised;

    // Map of named entity classes
    typedef std::map<std::string, Doom3EntityClassPtr> EntityClasses;
    EntityClasses _entityClasses;

    typedef std::map<std::string, Doom3ModelDefPtr> Models;
    Models _models;

    // The worker thread loading the eclasses will return this structure
    std::future<bool> _loadResult;

    bool _defsLoaded;

	// A unique parse pass identifier, used to check when existing
	// definitions have been parsed
	std::size_t _curParseStamp;

    sigc::signal<void> _defsReloadedSignal;

public:
    // Constructor
	EClassManager();

    // IEntityClassManager implementation
    sigc::signal<void> defsReloadedSignal() const override;
    virtual IEntityClassPtr findOrInsert(const std::string& name,
                                         bool has_brushes) override;
    IEntityClassPtr findClass(const std::string& className) override;
    virtual void forEachEntityClass(EntityClassVisitor& visitor) override;
    void realise() override;
    void unrealise() override;

    // VFS::Observer implementation
    virtual void onFileSystemInitialise();
    virtual void onFileSystemShutdown();

    // Find the modeldef with the given name
    virtual IModelDefPtr findModel(const std::string& name) override;
    virtual void forEachModelDef(ModelDefVisitor& visitor) override;

	// Reloads all entityDefs/modelDefs
    void reloadDefs() override;

    // RegisterableModule implementation
	virtual const std::string& getName() const override;
    virtual const StringSet& getDependencies() const override;
    virtual void initialiseModule(const ApplicationContext& ctx) override;
    virtual void shutdownModule() override;

	// Method loading the DEF files
    void parseFile(const std::string& filename);

private:
    // Since loading is happening in a worker thread, we need to ensure
    // that it's done loading before accessing any defs or models.
    void ensureDefsLoaded();

    // Worker function usually done in a separate thread
    bool loadDefAndResolveInheritance();

	// Tries to insert the given eclass, not overwriting existing ones
	// In either case, the eclass in the map is returned
	Doom3EntityClassPtr insertUnique(const Doom3EntityClassPtr& eclass);
    Doom3EntityClassPtr findInternal(const std::string& name);

	// Parses the given inputstream for DEFs.
	void parse(TextInputStream& inStr, const std::string& modDir);

	// Recursively resolves the inheritance of the model defs
	void resolveModelInheritance(const std::string& name, const Doom3ModelDefPtr& model);

	void parseDefFiles();
	void resolveInheritance();

	void reloadDefsCmd(const cmd::ArgumentList& args);
};
typedef std::shared_ptr<EClassManager> EClassManagerPtr;

} // namespace eclass
