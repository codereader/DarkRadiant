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
    ITargetKey::Ptr _key;

private:

	void _onModelButton(wxCommandEvent& ev);
	void _onSkinButton(wxCommandEvent& ev);
	void _onParticleButton(wxCommandEvent& ev);

public:

	// Main constructor
    ModelPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<ModelPropertyEditor>(parent, entities, key);
    }
};

} // namespace
