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
	void _onSkinButton(wxCommandEvent& ev);
	void _onParticleButton(wxCommandEvent& ev);

public:

	// Main constructor
    ModelPropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& name);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const std::string& name)
    {
        return std::make_shared<ModelPropertyEditor>(parent, entities, name);
    }
};

} // namespace
