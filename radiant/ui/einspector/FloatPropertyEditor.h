#pragma once

#include "PropertyEditor.h"

class wxSlider;

namespace ui
{

/**
 * Property editor for a ranged floating point value. A slider widget is used
 * to adjust the value between the upper and lower limit in the range. The
 * range information is passed in via the options string.
 */
class FloatPropertyEditor
: public PropertyEditor
{
private:
	// Slider widget
	wxSlider* _scale;

	// Name of key
	std::string _key;

private:

	void _onApply();

public:

	/**
	 * Default constructor for creation in the map.
	 */
	FloatPropertyEditor();

	/**
	 * Construct with Entity, key name and options.
	 */
	FloatPropertyEditor(wxWindow* parent, Entity*, const std::string&, const std::string&);

	/**
	 * Virtual PropertyEditor clone method.
	 */
	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    							const std::string& name,
    							const std::string& options)
	{
		return PropertyEditorPtr(
			new FloatPropertyEditor(parent, entity, name, options)
		);
	}

};

}
