#pragma once

#include "PropertyEditor.h"
#include <wx/event.h>

class wxCheckBox;
class wxCommandEvent;

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
	wxCheckBox* _checkBox;

	// Key to edit
	std::string _key;

private:

	void _onToggle(wxCommandEvent& ev);

public:

	// Construct a BooleanPropertyEditor with an entity and key to edit
	BooleanPropertyEditor(wxWindow* parent, Entity* entity, const std::string& name);

	// Construct a blank BooleanPropertyEditor for use in the
	// PropertyEditorFactory
	BooleanPropertyEditor();

	void updateFromEntity() override;

	// Create a new BooleanPropertyEditor
    virtual IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    									const std::string& name,
    									const std::string& options) override
	{
    	return PropertyEditorPtr(new BooleanPropertyEditor(parent, entity, name));
    }
};

} // namespace
