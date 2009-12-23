#ifndef ENTITYPROPERTYEDITOR_H_
#define ENTITYPROPERTYEDITOR_H_

#include "ComboBoxPropertyEditor.h"

namespace ui {

/**
 * Property editor which displays a button opening a separate dialog, filled with the
 * entities in the map. This is used for properties like "target" and "bind" which
 * should reference a different entity.
 */
class EntityPropertyEditor:
    public PropertyEditor
{    	
protected:
	// Main widget
	GtkWidget* _widget;
	
	// Keyvalue to set
	std::string _key;

public:
    // Construct a EntityPropertyEditor with an entity and key to edit
    EntityPropertyEditor(Entity* entity, const std::string& name);
    
    // Construct a blank EntityPropertyEditor for use in the 
    // PropertyEditorFactory
    EntityPropertyEditor();

    // Create a new EntityPropertyEditor
    virtual IPropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
        return PropertyEditorPtr(new EntityPropertyEditor(entity, name));
    }

protected:
	// Return main widget to parent class
	GtkWidget* _getWidget() const 
    {
		return _widget;
	}

private:
    static void _onBrowseButton(GtkWidget* w, EntityPropertyEditor* self);
};

}

#endif /*ENTITYPROPERTYEDITOR_H_*/
