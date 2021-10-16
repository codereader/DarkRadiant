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
	// Texture prefixes we are interested in
	const std::string _prefixes;

	// Keyval to set
	std::string _key;

private:

	void onBrowseButtonClick() override;

public:

	// Construct a TexturePropertyEditor with an entity and key to edit
    TexturePropertyEditor(wxWindow* parent, IEntitySelection& entities,
                          const std::string& name, const std::string& options);

	// Create a new TexturePropertyEditor
    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities,
                          const std::string& name, const std::string& options)
    {
        return std::make_shared<TexturePropertyEditor>(parent, entities, name, options);
    }
};

}
