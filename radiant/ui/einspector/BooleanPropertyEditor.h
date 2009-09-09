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
    // Containing widget
    GtkWidget* _widget;
    
	// The checkbox
	GtkWidget* _checkBox;
	
	// Key to edit
	std::string _key;
	
private:
	
	/* GTK CALLBACKS */
	static void _onToggle(GtkWidget*, BooleanPropertyEditor*);

protected:
	
	// Return main widget to parent class
	GtkWidget* _getWidget() const 
    {
		return _widget;
	}
	
public:

	// Construct a BooleanPropertyEditor with an entity and key to edit
	BooleanPropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank BooleanPropertyEditor for use in the 
	// PropertyEditorFactory
	BooleanPropertyEditor();

	// Create a new BooleanPropertyEditor
    virtual IPropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
    	return PropertyEditorPtr(new BooleanPropertyEditor(entity, name));
    }
};

}

#endif /*BOOLEANPROPERTYEDITOR_H_*/
