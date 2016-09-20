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

	// Name of key
	std::string _key;

private:

	void _onApply(wxCommandEvent& ev);

public:

	/**
	 * Default constructor for creation in the map.
	 */
	FloatPropertyEditor();

	/**
	 * Construct with Entity, key name and options.
	 */
	FloatPropertyEditor(wxWindow* parent, Entity*, const std::string&, const std::string&);

	void updateFromEntity() override;

	/**
	 * Virtual PropertyEditor clone method.
	 */
	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    							const std::string& name,
    							const std::string& options) override
	{
		return PropertyEditorPtr(
			new FloatPropertyEditor(parent, entity, name, options)
		);
	}

};

}
