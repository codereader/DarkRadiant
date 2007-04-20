#ifndef BOOLEANPROPERTYEDITOR_H_
#define BOOLEANPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/* BooleanPropertyEditor
 * 
 * PropertyEditor that displays and edits a boolean (toggle) value
 */

class BooleanPropertyEditor:
    public PropertyEditor
{
	// The checkbox
	GtkWidget* _checkBox;
	
	// Entity to edit
	Entity* _entity;

public:

	// Construct a BooleanPropertyEditor with an entity and key to edit
	BooleanPropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank BooleanPropertyEditor for use in the PropertyEditorFactory
	BooleanPropertyEditor();

	// Create a new BooleanPropertyEditor
    virtual PropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
    	return PropertyEditorPtr(new BooleanPropertyEditor(entity, name));
    }
};

}

#endif /*BOOLEANPROPERTYEDITOR_H_*/
