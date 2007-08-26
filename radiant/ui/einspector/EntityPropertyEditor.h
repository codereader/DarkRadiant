#ifndef ENTITYPROPERTYEDITOR_H_
#define ENTITYPROPERTYEDITOR_H_

#include "PropertyEditor.h"

// Forward Declaration
typedef struct _GtkComboBox GtkComboBox;

namespace ui
{

/* Property editor which displays an editable combo box populated with all of the other
 * Entities in the map. This is used for properties like "target" and "bind" which
 * should reference a different entity.
 */

class EntityPropertyEditor:
    public PropertyEditor
{
	// The combo box
    GtkWidget* _comboBox;
    
    // Entity to edit
	Entity* _entity;
	
	// Name of keyval
	std::string _key;
    	
private:

    // Populate the combo box by traversing the scenegraph for Entities
    void populateComboBox();
    
    // GTK Callback for combo box selection changes
    static void onSelectionChange(GtkComboBox* widget, EntityPropertyEditor* self);
    
public:

    // Construct a EntityPropertyEditor with an entity and key to edit
    EntityPropertyEditor(Entity* entity, const std::string& name);
    
    // Construct a blank EntityPropertyEditor for use in the 
    // PropertyEditorFactory
    EntityPropertyEditor();

    // Create a new EntityPropertyEditor
    virtual PropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
        return PropertyEditorPtr(new EntityPropertyEditor(entity, name));
    }
  
};

}

#endif /*ENTITYPROPERTYEDITOR_H_*/
