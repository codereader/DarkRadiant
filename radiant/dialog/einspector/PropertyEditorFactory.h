#ifndef PROPERTYEDITORFACTORY_H_
#define PROPERTYEDITORFACTORY_H_

#include "PropertyEditor.h"
#include "ientity.h"

#include <map>
#include <string>
#include <iostream>

namespace ui
{

/* PropertyEditorFactory
 * 
 * Factory class to create PropertyEditor child classes based on the classnames
 * provided at runtime.
 */

class PropertyEditorFactory
{
   
   // Mapping from classnames to PropertyEditor child instances. The child
   // instance's createNew() function will be used to create a new object of
   // the correct type.
   static std::map<const std::string, PropertyEditor*> _peMap; 
    
public:

    // Create a new PropertyEditor with the provided classname to manage the
    // given Entity object and key name.
    static PropertyEditor* create(const std::string className, Entity* entity, const char* key) {
    	PropertyEditor *pe = _peMap[className];
    	if (pe) {
    		return pe->createNew(entity, key);
    	}
    	else {
    		std::cout << "Failed to find \"" << className << "\"" << std::endl;
    		return NULL;
    	}
    	
    }

    // Register a new PropertyEditor derivative into the internal mapping.
    static void registerClass(const std::string name, PropertyEditor* editor) {
        _peMap[name] = editor;
    } 

};

}

#endif /*PROPERTYEDITORFACTORY_H_*/
