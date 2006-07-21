#ifndef CLASSNAMEPROPERTYEDITOR_H_
#define CLASSNAMEPROPERTYEDITOR_H_

#include "PropertyEditor.h"

#include "ientity.h"

#include <gtk/gtk.h>

#include <string>


namespace ui
{

/* Property editor which displays a combobox populated with all of the available
 * EntityClasses in the map. Selecting an invalid EntityClass is not possible.
 */

class ClassnamePropertyEditor:
    public PropertyEditor
{
private:

    // The combo box
    GtkWidget* _comboBox;
    
private:

    // Populate the combo box by traversing the scenegraph for Entities
    void populateComboBox();
    
public:

    // Construct a ClassnamePropertyEditor with an entity and key to edit
    ClassnamePropertyEditor(Entity* entity, const std::string& name);
    
    // Construct a blank ClassnamePropertyEditor for use in the PropertyEditorFactory
    ClassnamePropertyEditor();

    // Create a new ClassnamePropertyEditor
    virtual PropertyEditor* createNew(Entity* entity, const std::string& name) {
        return new ClassnamePropertyEditor(entity, name);
    }
    
    virtual void setValue(const std::string&);
    virtual const std::string getValue();
};

} // namespace ui

#endif /*CLASSNAMEPROPERTYEDITOR_H_*/
