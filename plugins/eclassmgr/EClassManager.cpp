#include "EClassManager.h"

#include "iregistry.h"
#include "irender.h"
#include "iuimanager.h"
#include "ifilesystem.h"
#include "archivelib.h"
#include "parser/DefTokeniser.h"

#include "Doom3EntityClass.h"
#include "Doom3ModelDef.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace eclass {

// Constructor
EClassManager::EClassManager() :
    _realised(false)
{}

// Get a named entity class, creating if necessary
IEntityClassPtr EClassManager::findOrInsert(const std::string& name, bool has_brushes) {
	// Return an error if no name is given
    if (name.empty()) {
        return IEntityClassPtr();
    }

	// Convert string to lowercase, for case-insensitive lookup
	std::string lName = boost::algorithm::to_lower_copy(name);

    // Find the EntityClass in the map.
    EntityClasses::iterator i = _entityClasses.find(lName);
    if (i != _entityClasses.end()) {
        return i->second; // found it, return
    }

    // Otherwise insert the new EntityClass
    //IEntityClassPtr e = eclass::Doom3EntityClass::create(lName, has_brushes);
    // greebo: Changed fallback behaviour when unknown entites are encountered to TRUE
    // so that brushes of unknown entites don't get lost (issue #240)
    IEntityClassPtr e = Doom3EntityClass::create(lName, true);
    
    // Try to insert the class
    return insertUnique(e);
}

IEntityClassPtr EClassManager::insertUnique(IEntityClassPtr eclass) {
	// Try to insert the eclass
    std::pair<EntityClasses::iterator, bool> i = _entityClasses.insert(
    	EntityClasses::value_type(eclass->getName(), eclass)
    );
    
    // Return the pointer to the inserted eclass
    return i.first->second;
}

void EClassManager::resolveModelInheritance(const std::string& name, IModelDefPtr& model) {
	if (model->resolved == true) {
		return; // inheritance already resolved
	}
	
	model->resolved = true;

	if (!model->parent.empty()) {
		Models::iterator i = _models.find(model->parent);
		
		if (i == _models.end()) {
			globalErrorStream() << "model " << name
					<< " inherits unknown model " << model->parent << "\n";
		} 
		else {
			resolveModelInheritance(i->first, i->second);
			model->mesh = i->second->mesh;
			model->skin = i->second->skin;
		}
	}
}

void EClassManager::realise() {
	if (_realised) {
		return; // nothing to do anymore
	}

	globalOutputStream() << "searching vfs directory 'def' for *.def\n";
	GlobalFileSystem().forEachFile("def/", "def", LoadFileCaller(*this));

    // Resolve inheritance on the model classes
    for (Models::iterator i = _models.begin(); i != _models.end(); ++i) {
    	resolveModelInheritance(i->first, i->second);
    }
        
    // Resolve inheritance for the entities. At this stage the classes
    // will have the name of their parent, but not an actual pointer to
    // it        
    for (EntityClasses::iterator i = _entityClasses.begin();
         i != _entityClasses.end(); ++i) 
	{
		// Get a Doom3EntityClass pointer
		Doom3EntityClassPtr d3c = boost::static_pointer_cast<Doom3EntityClass>(i->second);
			
		// Tell the class to resolve its own inheritance using the given
		// map as a source for parent lookup
		d3c->resolveInheritance(_entityClasses);

        // If the entity has a model path ("model" key), lookup the actual
        // model and apply its mesh and skin to this entity.
        if (i->second->getModelPath().size() > 0) {
            Models::iterator j = _models.find(i->second->getModelPath());
            if (j != _models.end()) {
                i->second->setModelPath(j->second->mesh);
                i->second->setSkin(j->second->skin);
            }
        }
    }
    
    // greebo: Override the eclass colours of two special entityclasses
    Vector3 worlspawnColour = ColourSchemes().getColour("default_brush");
    Vector3 lightColour = ColourSchemes().getColour("light_volumes");
    
    IEntityClassPtr light = findOrInsert("light", true);
	light->setColour(lightColour);
	
	IEntityClassPtr worldspawn = findOrInsert("worldspawn", true);
	worldspawn->setColour(worlspawnColour);

	_realised = true;
}

// Find an entity class
IEntityClassPtr EClassManager::findClass(const std::string& className) const {
	// greebo: Convert the lookup className string to lowercase first
	std::string classNameLower = boost::algorithm::to_lower_copy(className);

    EntityClasses::const_iterator i = _entityClasses.find(classNameLower);
    if (i != _entityClasses.end()) {
        return i->second;
    }
    else {
        return IEntityClassPtr();
    }
}

// Visit each entity class
void EClassManager::forEach(EntityClassVisitor& visitor) {
	for(EntityClasses::iterator i = _entityClasses.begin(); 
		i != _entityClasses.end(); 
		++i)
	{
		visitor.visit(i->second);
	}
}

void EClassManager::unrealise() {
    if (_realised) {
       	_entityClasses.clear();
       	_realised = false;
    }
}

IModelDefPtr EClassManager::findModel(const std::string& name) const {
	Models::const_iterator found = _models.find(name);
	return (found != _models.end()) ? found->second : IModelDefPtr();
}

// RegisterableModule implementation
const std::string& EClassManager::getName() const {
	static std::string _name(MODULE_ECLASSMANAGER);
	return _name;
}

const StringSet& EClassManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_SHADERCACHE);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void EClassManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "EntityClassDoom3::initialiseModule called.\n";
	
	GlobalFileSystem().addObserver(*this);
	realise();
}

void EClassManager::shutdownModule() {
	globalOutputStream() << "EntityClassDoom3::shutdownModule called.\n";
	unrealise();
	GlobalFileSystem().removeObserver(*this);
}

// Gets called on VFS initialise
void EClassManager::onFileSystemInitialise() {
	realise();
}

// Gets called on VFS shutdown
void EClassManager::onFileSystemShutdown() {
	unrealise();
}

// Parse the provided stream containing the contents of a single .def file.
// Extract all entitydefs and create objects accordingly.
void EClassManager::parse(TextInputStream& inStr, const std::string& modDir) {
	// Construct a tokeniser for the stream
	std::istream is(&inStr);
    parser::BasicDefTokeniser<std::istream> tokeniser(is);

    while (tokeniser.hasMoreTokens()) {
        std::string blockType = tokeniser.nextToken();
        boost::algorithm::to_lower(blockType);

        if (blockType == "entitydef") {
        	// Allocate a new class
        	Doom3EntityClassPtr entityClass(new eclass::Doom3EntityClass(""));
        	
        	// Parse the contents of the eclass (including name)
        	entityClass->parseFromTokens(tokeniser);
        	
        	entityClass->setModName(modDir);
        	insertUnique(entityClass);
        }
        else if(blockType == "model") {
            // Allocate an empty ModelDef 	
        	Doom3ModelDefPtr model(new Doom3ModelDef);
        	
        	// Invoke the parser routine
        	model->parseFromTokens(tokeniser);
        	
			std::pair<Models::iterator, bool> result = _models.insert(
				Models::value_type(model->name, model)
			);

			// Was the model already in the map (bool == false)?
			if (!result.second) {
		    	std::cout << "[eclassmgr]: Model " << model->name << " redefined.\n";
		    }
        }
    }
}

void EClassManager::loadFile(const std::string& filename) {
    const std::string fullname = "def/" + filename;

	ArchiveTextFilePtr file = GlobalFileSystem().openTextFile(fullname);
	
	if (file != NULL) {
        try {
            // Parse entity defs from the file
            parse(file->getInputStream(), file->getModName());
        }
        catch (parser::ParseException e) {
            std::cerr << "[eclassmgr] failed to parse " << filename 
            		  << " (" << e.what() << ")" << std::endl;
        }
	}
}

} // namespace eclass
