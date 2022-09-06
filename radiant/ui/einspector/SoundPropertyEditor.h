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
    ITargetKey::Ptr _key;

private:

	void onBrowseButtonClick() override;

public:

	// Main constructor
    SoundPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<SoundPropertyEditor>(parent, entities, key);
    }
};

}
