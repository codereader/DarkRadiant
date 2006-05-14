#ifndef PROPERTYCATEGORY_H_
#define PROPERTYCATEGORY_H_

#include <string>
#include <map>
#include <iostream>

namespace ui
{

/* PropertyCategory
 * 
 * A PropertyCategory represents a top-level grouping of properties in the Entity
 * Inspector, for instance "Basic", "AI", "Light" etc. Each PropertyCategory maintains
 * a list of key names, and their mappings to a PropertyEditor type.
 * 
 * E.g. PropertyCategory "Basic"
 * 			"origin" -> "vector3"
 * 			"classname" -> "text"
 * 
 * The set of PropertyCategories corresponds to the equivalent nodes in the .game
 * file which is parsed by the EntityInspector class.
 */

class PropertyCategory
{
	// The name of this PropertyCategory
	std::string _name;
	
	// Mapping of key names to PropertyEditor types
	std::map<std::string, std::string> _propertyMap;
	
public:

	// Constructor sets the category name
	PropertyCategory(std::string name) {
		std::cout << "PropertyCategory " << name << " created." << std::endl;
		_name = name;
	}
	
	// Add a key name to this category.
	void add(std::string keyName, std::string type) {
		assert(keyName.size() > 0);
		assert(type.size() > 0);
		_propertyMap[keyName] = type;
	}
	
	// Lookup the PropertyEditor type that corresponds to the supplied key name
	std::string get(std::string keyName) {
		assert(keyName.size() > 0);
		return _propertyMap.find(keyName)->second;
	}
	
};

}

#endif /*PROPERTYCATEGORY_H_*/
