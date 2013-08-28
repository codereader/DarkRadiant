#ifndef ECLASSMANAGER_H_
#define ECLASSMANAGER_H_

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
    public VirtualFileSystem::Observer,
	public VirtualFileSystem::Visitor
{
    // Whether the entity classes have been realised
    bool _realised;

    // Map of named entity classes
    typedef std::map<std::string, Doom3EntityClassPtr> EntityClasses;
    EntityClasses _entityClasses;

    typedef std::map<std::string, Doom3ModelDefPtr> Models;
    Models _models;

	// A unique parse pass identifier, used to check when existing
	// definitions have been parsed
	std::size_t _curParseStamp;

    sigc::signal<void> _defsReloadedSignal;

public:
    // Constructor
	EClassManager();

    // IEntityClassManager implementation
    sigc::signal<void> defsReloadedSignal() const;
    virtual IEntityClassPtr findOrInsert(const std::string& name,
    									 bool has_brushes);
    IEntityClassPtr findClass(const std::string& className) const;
	virtual void forEachEntityClass(EntityClassVisitor& visitor);
	void realise();
    void unrealise();

    // VFS::Observer implementation
    virtual void onFileSystemInitialise();
    virtual void onFileSystemShutdown();

    // Find the modeldef with the given name
    virtual IModelDefPtr findModel(const std::string& name) const;
	virtual void forEachModelDef(ModelDefVisitor& visitor);

	// Reloads all entityDefs/modelDefs
	void reloadDefs();

    // RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

	// Method loading the DEF files (gets called by GlobalFilesystem().foreach()).
	void visit(const std::string& filename);

private:
	// Tries to insert the given eclass, not overwriting existing ones
	// In either case, the eclass in the map is returned
	Doom3EntityClassPtr insertUnique(const Doom3EntityClassPtr& eclass);
    Doom3EntityClassPtr findInternal(const std::string& name) const;

	// Parses the given inputstream for DEFs.
	void parse(TextInputStream& inStr, const std::string& modDir);

	// Recursively resolves the inheritance of the model defs
	void resolveModelInheritance(const std::string& name, const Doom3ModelDefPtr& model);

	void parseDefFiles();
	void resolveInheritance();

	void reloadDefsCmd(const cmd::ArgumentList& args);
};
typedef boost::shared_ptr<EClassManager> EClassManagerPtr;

} // namespace eclass

#endif /*ECLASSMANAGER_H_*/
