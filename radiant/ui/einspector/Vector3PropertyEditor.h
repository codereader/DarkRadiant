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
    // Main widget
    GtkWidget* _widget;
    
	// The 3 component fields.
	GtkWidget* _xValue;
    GtkWidget* _yValue;
    GtkWidget* _zValue;
    
    // Entity to edit
    Entity* _entity;
    
    // Name of key
    std::string _key;
    
private:

	// Set the spinbox contents from the keyvalue
	void setWidgetsFromKey(const std::string& value);

	/* GTK CALLBACKS */
	static void _onApply(GtkWidget*, Vector3PropertyEditor*);

protected:
	
	// Return main widget to parent class
	GtkWidget* _getInternalWidget() {
		return _widget;
	}
	
public:

	// Construct a TextPropertyEditor with an entity and key to edit
	Vector3PropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank TextPropertyEditor for use in the PropertyEditorFactory
	Vector3PropertyEditor();
	
	// Create a new TextPropertyEditor
    virtual PropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
    	return PropertyEditorPtr(new Vector3PropertyEditor(entity, name));
    }

};

}

#endif /*VECTOR3PROPERTYEDITOR_H_*/
