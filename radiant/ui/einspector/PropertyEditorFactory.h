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
    static PropertyEditor* create(const std::string& className, Entity* entity, const std::string& key);

    // Register the classes
    static void registerClasses();
    
    // Return the GdkPixbuf that corresponds to the provided PropertyEditor
    // type.
    static GdkPixbuf* getPixbufFor(std::string type);

};

}

#endif /*PROPERTYEDITORFACTORY_H_*/
