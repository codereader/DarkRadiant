#ifndef COMPONENTEDITORFACTORY_H_
#define COMPONENTEDITORFACTORY_H_

#include "ComponentEditor.h"

#include <map>
#include <string>

namespace objectives
{

namespace ce
{

// Map of named ComponentEditor subclasses
typedef std::map<std::string, ComponentEditorPtr> ComponentEditorMap;

/**
 * Factory class which creates ComponentEditor subclasses based on a named
 * Component type.
 */
class ComponentEditorFactory
{
	static ComponentEditorMap& getMap();
	
public:
	
	/**
	 * Create a ComponentEditor of the named type.
	 * 
	 * @param type
	 * The string type of the ComponentEditor which should be returned.
	 * 
	 * @return
	 * A shared pointer to a ComponentEditor of the requested type.
	 * 
	 * @exception std::logic_error
	 * Thrown if the named type is not recognised. Since this method is only
	 * intended to be invoked by the ComponentsDialog, this indicates a logic
	 * error.
	 * 
	 */
	static ComponentEditorPtr create(const std::string& type);
	
	/**
	 * Register a named ComponentEditor subclass. This is intended for use by
	 * ComponentEditor subclasses to register themselves during static
	 * initialisation, thereby populating the map of instances used for virtual
	 * construction.
	 * 
	 * @param type
	 * The named type to register.
	 * 
	 * @param subclass
	 * An instance of the ComponentEditor subclass used for virtual
	 * construction.
	 */
	static void registerType(const std::string& type, 
							 ComponentEditorPtr subclass);
};

}

}

#endif /*COMPONENTEDITORFACTORY_H_*/
