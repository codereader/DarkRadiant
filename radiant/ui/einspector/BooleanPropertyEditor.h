#ifndef BOOLEANPROPERTYEDITOR_H_
#define BOOLEANPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace Gtk { class CheckButton; }

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
	Gtk::CheckButton* _checkBox;

	// Key to edit
	std::string _key;

private:

	void _onToggle();

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
