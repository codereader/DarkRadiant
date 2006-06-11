#ifndef ENTITYPROPERTYEDITOR_H_
#define ENTITYPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/* Property editor which displays an editable combo box populated with all of the other
 * Entities in the map. This is used for properties like "target" and "bind" which
 * should reference a different entity.
 */

class EntityPropertyEditor:
    public PropertyEditor
{
private:

    // The combo box
    GtkWidget* _comboBox;
    
public:

    // Construct a EntityPropertyEditor with an entity and key to edit
    EntityPropertyEditor(Entity* entity, const std::string& name);
    
    // Construct a blank EntityPropertyEditor for use in the PropertyEditorFactory
    EntityPropertyEditor();

    // Create a new EntityPropertyEditor
    virtual PropertyEditor* createNew(Entity* entity, const std::string& name) {
        return new EntityPropertyEditor(entity, name);
    }
    
    virtual void setValue(const std::string&);
    virtual const std::string getValue();
};

}

#endif /*ENTITYPROPERTYEDITOR_H_*/
