#ifndef ECLASSMANAGER_H_
#define ECLASSMANAGER_H_

#include "ieclass.h"
#include "icommandsystem.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "moduleobservers.h"
#include "generic/callback.h"

#include "Doom3EntityClass.h"
#include "Doom3ModelDef.h"

namespace eclass {

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

	// A unique parse pass identifier, used to check when existing
	// definitions have been parsed
	std::size_t _curParseStamp;

	typedef std::set<IEntityClassManager::Observer*> Observers;
	Observers _observers;

public:
    // Constructor
	EClassManager(); 

	// Add or remove an observer to get notified on eclass events
	virtual void addObserver(IEntityClassManager::Observer* observer);
	virtual void removeObserver(IEntityClassManager::Observer* observer);
    
    // Get a named entity class, creating if necessary
    virtual IEntityClassPtr findOrInsert(const std::string& name,
    									 bool has_brushes);
    
    // Find an entity class
    IEntityClassPtr findClass(const std::string& className) const;
    
    // Visit each entity class
	virtual void forEach(EntityClassVisitor& visitor);
    
	void realise();
    void unrealise();
    
    // VFS::Observer implementation
    virtual void onFileSystemInitialise();
    virtual void onFileSystemShutdown();

    // Find the modeldef with the given name
    virtual IModelDefPtr findModel(const std::string& name) const;

	// Reloads all entityDefs/modelDefs
	void reloadDefs();
    
    // RegisterableModule implementation
	virtual const std::string& getName() const;	
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);	
	virtual void shutdownModule();
	
	// Functor loading the DEF files (gets called by GlobalFilesystem().foreach()).
	void loadFile(const std::string& filename);
	typedef MemberCaller1<EClassManager, const std::string&, &EClassManager::loadFile> LoadFileCaller;

private:
	// Tries to insert the given eclass, not overwriting existing ones
	// In either case, the eclass in the map is returned 
	Doom3EntityClassPtr insertUnique(const Doom3EntityClassPtr& eclass);
	
	// Parses the given inputstream for DEFs.
	void parse(TextInputStream& inStr, const std::string& modDir);
	
	// Recursively resolves the inheritance of the model defs
	void resolveModelInheritance(const std::string& name, const Doom3ModelDefPtr& model);

	void parseDefFiles();
	void resolveInheritance();
};
typedef boost::shared_ptr<EClassManager> EClassManagerPtr;

} // namespace eclass

#endif /*ECLASSMANAGER_H_*/
