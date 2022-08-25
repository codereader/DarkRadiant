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

	// Construct a BooleanPropertyEditor with a key to edit
	BooleanPropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& name);

	void updateFromEntity();

	// Create a new BooleanPropertyEditor
    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const std::string& name)
    {
        return std::make_shared<BooleanPropertyEditor>(parent, entities, name);
    }
};

} // namespace
