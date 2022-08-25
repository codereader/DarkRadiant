#pragma once

#include "PropertyEditor.h"

namespace ui
{

/** PropertyEditor that allows selection of a valid texture. The property
 * editor will display an editable text field along with a "browse" button
 * which will pop up a Tree View where a texture can be selected.
 */

class TexturePropertyEditor : 
	public PropertyEditor
{
private:
	// Keyval to set
	std::string _key;

private:

	void onBrowseButtonClick() override;

public:

	// Construct a TexturePropertyEditor with an entity and key to edit
    TexturePropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& name);

	// Create a new TexturePropertyEditor
    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const std::string& name)
    {
        return std::make_shared<TexturePropertyEditor>(parent, entities, name);
    }
};

}
