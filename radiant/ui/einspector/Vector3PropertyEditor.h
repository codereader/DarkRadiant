#ifndef VECTOR3PROPERTYEDITOR_H_
#define VECTOR3PROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace Gtk { class SpinButton; }

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
	Gtk::SpinButton* _xValue;
    Gtk::SpinButton* _yValue;
    Gtk::SpinButton* _zValue;

    // Name of key
    std::string _key;

private:

	// Set the spinbox contents from the keyvalue
	void setWidgetsFromKey(const std::string& value);

	// gtkmm callback
	void _onApply();

public:
	// Construct a TextPropertyEditor with an entity and key to edit
	Vector3PropertyEditor(Entity* entity, const std::string& name);

	// Construct a blank TextPropertyEditor for use in the PropertyEditorFactory
	Vector3PropertyEditor();

	// Create a new TextPropertyEditor
    virtual IPropertyEditorPtr createNew(Entity* entity,
    									const std::string& name,
    									const std::string& options)
	{
    	return PropertyEditorPtr(new Vector3PropertyEditor(entity, name));
    }
};

}

#endif /*VECTOR3PROPERTYEDITOR_H_*/
