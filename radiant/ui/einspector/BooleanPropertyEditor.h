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
	// Main widget
	GtkWidget* _widget;

	// The checkbox
	GtkWidget* _checkBox;
	
	// Entity to edit
	Entity* _entity;

public:

	// Construct a BooleanPropertyEditor with an entity and key to edit
	BooleanPropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank BooleanPropertyEditor for use in the PropertyEditorFactory
	BooleanPropertyEditor();

	// Destructor
	virtual ~BooleanPropertyEditor() {
		gtk_widget_destroy(_widget);
	}

	// Create a new BooleanPropertyEditor
    virtual PropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
    	return PropertyEditorPtr(new BooleanPropertyEditor(entity, name));
    }
  
  	/**
  	 * Get the main widget.
  	 */
	GtkWidget* getWidget() {
		return _widget;
	}
    
};

}

#endif /*BOOLEANPROPERTYEDITOR_H_*/
