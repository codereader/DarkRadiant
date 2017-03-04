#pragma once

#include "PropertyEditor.h"

namespace ui
{

/**
 * Property editor for selecting a sound shader.
 */
class SoundPropertyEditor : 
	public PropertyEditor
{
private:
	// Keyvalue to set
	std::string _key;

private:

	void onBrowseButtonClick() override;

public:

	// Default constructor for the map
	SoundPropertyEditor() { }

	// Main constructor
	SoundPropertyEditor(wxWindow* parent, Entity* entity,
					    const std::string& name,
					    const std::string& options);

	// Clone method for virtual construction
	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
								const std::string& name,
								const std::string& options) override
	{
		return PropertyEditorPtr(
			new SoundPropertyEditor(parent, entity, name, options)
		);
	}
};

}
