#ifndef VECTOR3PROPERTYEDITOR_H_
#define VECTOR3PROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/* Vector3PropertyEditor
 * 
 * PropertyEditor that displays and edits a 3-dimensional vector value such as
 * an origin or radius.
 */

class Vector3PropertyEditor:
    public PropertyEditor
{
private:

	// The 3 component fields.
	GtkWidget* _xValue;
    GtkWidget* _yValue;
    GtkWidget* _zValue;

public:

	// Construct a TextPropertyEditor with an entity and key to edit
	Vector3PropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank TextPropertyEditor for use in the PropertyEditorFactory
	Vector3PropertyEditor();

	// Create a new TextPropertyEditor
    virtual PropertyEditor* createNew(Entity* entity, const std::string& name) {
    	return new Vector3PropertyEditor(entity, name);
    }
    
    virtual void setValue(const std::string&);
    virtual const std::string getValue();
    
};

}

#endif /*VECTOR3PROPERTYEDITOR_H_*/
