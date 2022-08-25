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

	// Main constructor
    SoundPropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& name);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const std::string& name)
    {
        return std::make_shared<SoundPropertyEditor>(parent, entities, name);
    }
};

}
