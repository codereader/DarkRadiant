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
	TexturePropertyEditor(wxWindow* parent, Entity* entity,
						  const std::string& name,
						  const std::string& options);

	// Construct a blank TexturePropertyEditor for use in the
	// PropertyEditorFactory
	TexturePropertyEditor() {}

	// Create a new TexturePropertyEditor
    virtual IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    									const std::string& name,
    									const std::string& options) override
	{
    	return PropertyEditorPtr(
    		new TexturePropertyEditor(parent, entity, name, options)
    	);
    }
};

}
