#ifndef PROPERTYEDITORREGISTRATIONHELPER_H_
#define PROPERTYEDITORREGISTRATIONHELPER_H_

#include "PropertyEditor.h"
#include "PropertyEditorFactory.h"

#include <string>

namespace ui
{

/* PropertyEditorRegistrationHelper
 * 
 * This is a helper class which is added to PropertyEditor subclasses as a
 * static member field. On instantiation, the PropertyEditorRegistrationHelper
 * adds its containing PropertyEditor derivative to the PropertyEditorFactory's 
 * mapping from names to classes, allowing runtime instantiation of
 * PropertyEditor derivatives based on their classnames.
 */

class PropertyEditorRegistrationHelper
{
public:
    // Constructor takes the string name and a const instance of the derived
    // PropertyEditor class, for future calls to createNew(), and instructs the
    // PropertyEditor base class to register the subclass in its table.
	PropertyEditorRegistrationHelper(const std::string name, PropertyEditor* editor) {
            PropertyEditorFactory::registerClass(name, editor);
    }
};

}

#endif /*PROPERTYEDITORREGISTRATIONHELPER_H_*/
