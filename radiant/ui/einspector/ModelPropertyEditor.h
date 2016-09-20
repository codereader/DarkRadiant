#pragma once

#include <wx/event.h>

#include "PropertyEditor.h"

namespace ui
{

/**
 * Property editor for "model" keys. Displays a browse button that can be used
 * to select a model using the Model Selector.
 */
class ModelPropertyEditor : 
	public PropertyEditor
{
private:
	// Keyvalue to set
	std::string _key;

private:

	void _onModelButton(wxCommandEvent& ev);
	void _onParticleButton(wxCommandEvent& ev);

public:

	// Default constructor for the map
	ModelPropertyEditor();

	// Main constructor
	ModelPropertyEditor(wxWindow* parent, Entity* entity,
					    const std::string& name,
					    const std::string& options);

	// Clone method for virtual construction
	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
								const std::string& name,
								const std::string& options) override
	{
		return PropertyEditorPtr(
			new ModelPropertyEditor(parent, entity, name, options)
		);
	}
};

} // namespace
