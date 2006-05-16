#ifndef PROPERTYEDITORFACTORY_H_
#define PROPERTYEDITORFACTORY_H_

#include "PropertyEditor.h"
#include "ientity.h"

#include "gtkutil/dialog.h"

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
   typedef std::map<const std::string, PropertyEditor*> PropertyEditorMap;
   static PropertyEditorMap _peMap; 
    
public:

    // Create a new PropertyEditor with the provided classname to manage the
    // given Entity object and key name.
    static PropertyEditor* create(const std::string& className, Entity* entity, const std::string& key) {
		PropertyEditorMap::iterator iter(_peMap.find(className));
		if (iter == _peMap.end()) {
			gtkutil::errorDialog(std::string("PropertyEditorFactory: unable to find PropertyEditor instance") 
								+ " for type \"" + className + "\".");
			return NULL;
		} else {
			return iter->second->createNew(entity, key);
    	}
    }

    // Register a new PropertyEditor derivative into the internal mapping.
    static void registerClass(const std::string name, PropertyEditor* editor) {
        _peMap[name] = editor;
    } 

};

}

#endif /*PROPERTYEDITORFACTORY_H_*/
