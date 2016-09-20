#pragma once

#include <wx/event.h>
#include "PropertyEditor.h"

class wxSpinCtrl;

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
	wxSpinCtrl* _xValue;
    wxSpinCtrl* _yValue;
    wxSpinCtrl* _zValue;

    // Name of key
    std::string _key;

private:

	// Set the spinbox contents from the keyvalue
	void setWidgetsFromKey(const std::string& value);

	void _onApply(wxCommandEvent& ev);

public:
	// Construct a TextPropertyEditor with an entity and key to edit
	Vector3PropertyEditor(wxWindow* parent, Entity* entity, const std::string& name);

	// Construct a blank TextPropertyEditor for use in the PropertyEditorFactory
	Vector3PropertyEditor();

	void updateFromEntity() override;

	// Create a new TextPropertyEditor
    virtual IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    									const std::string& name,
    									const std::string& options) override
	{
    	return PropertyEditorPtr(new Vector3PropertyEditor(parent, entity, name));
    }
};

}
