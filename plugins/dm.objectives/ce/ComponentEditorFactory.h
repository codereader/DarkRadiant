#ifndef COMPONENTEDITORFACTORY_H_
#define COMPONENTEDITORFACTORY_H_

#include "ComponentEditor.h"

#include <map>
#include <string>

namespace objectives
{

class Component;

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
	// Map instance
	static ComponentEditorMap& getMap();
	
public:
	
	/**
	 * Create a ComponentEditor of the named type.
	 * 
	 * @param type
	 * The string type of the ComponentEditor which should be returned.
	 * 
	 * @param component
	 * A reference to the Component that the new ComponentEditor will edit.
	 * 
	 * @return
	 * A shared pointer to a ComponentEditor of the requested type. If the
	 * requested type does not exist, a NULL shared pointer is returned.
	 */
	static ComponentEditorPtr create(const std::string& type,
									 objectives::Component& component);
	
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
