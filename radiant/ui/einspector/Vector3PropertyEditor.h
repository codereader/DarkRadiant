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

    // The target key
    ITargetKey::Ptr _key;

private:

	// Set the spinbox contents from the keyvalue
	void setWidgetsFromKey(const std::string& value);

	void _onApply(wxCommandEvent& ev);

public:
	// Construct a TextPropertyEditor with an entity and key to edit
	Vector3PropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

	void updateFromEntity();

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<Vector3PropertyEditor>(parent, entities, key);
    }
};

}
