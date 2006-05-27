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
private:

	// The checkbox
	GtkWidget* _checkBox;

public:

	// Construct a BooleanPropertyEditor with an entity and key to edit
	BooleanPropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank BooleanPropertyEditor for use in the PropertyEditorFactory
	BooleanPropertyEditor();

	virtual ~BooleanPropertyEditor();

	// Create a new BooleanPropertyEditor
    virtual PropertyEditor* createNew(Entity* entity, const std::string& name) {
    	return new BooleanPropertyEditor(entity, name);
    }
    
    virtual void setValue(const std::string&);
    virtual const std::string getValue();
    
};

}

#endif /*BOOLEANPROPERTYEDITOR_H_*/
