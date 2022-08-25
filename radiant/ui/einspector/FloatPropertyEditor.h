#pragma once

#include "PropertyEditor.h"
#include <wx/event.h>

class wxSpinCtrlDouble;

namespace ui
{

/**
 * Property editor for a ranged floating point value. A slider widget is used
 * to adjust the value between the upper and lower limit in the range. The
 * range information is passed in via the options string.
 */
class FloatPropertyEditor :
	public PropertyEditor
{
private:
	wxSpinCtrlDouble* _spinCtrl;

	// The target key
    ITargetKey::Ptr _key;

private:

	void _onApply(wxCommandEvent& ev);

public:

	/**
	 * Construct with Entity, key name and options.
	 */
	FloatPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

	void updateFromEntity();

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<FloatPropertyEditor>(parent, entities, key);
    }

};

}
