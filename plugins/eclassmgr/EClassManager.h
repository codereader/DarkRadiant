#ifndef ECLASSMANAGER_H_
#define ECLASSMANAGER_H_

#include "ieclass.h"
#include "ifilesystem.h"
#include "moduleobservers.h"
#include "generic/callback.h"

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
    typedef std::map<std::string, IEntityClassPtr> EntityClasses;
    EntityClasses _entityClasses;
    
    typedef std::map<std::string, IModelDefPtr> Models;
    Models _models;

public:
    // Constructor
	EClassManager(); 
    
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
	IEntityClassPtr insertUnique(IEntityClassPtr eclass);
	
	// Parses the given inputstream for DEFs.
	void parse(TextInputStream& inStr, const std::string& modDir);
	
	// Recursively resolves the inheritance of the model defs
	void resolveModelInheritance(const std::string& name, IModelDefPtr& model);
};
typedef boost::shared_ptr<EClassManager> EClassManagerPtr;

} // namespace eclass

#endif /*ECLASSMANAGER_H_*/
