#pragma once

#include "PropertyEditor.h"

namespace ui
{

/**
 * Property Editor for the "skin" key of models. This contains a text entry box
 * with a browse button that displays a SkinChooser, which is a dialog that
 * allows the selection of both matching and generic skins to apply to the given
 * model.
 */
class SkinPropertyEditor : 
	public PropertyEditor,
	public IPropertyEditorDialog
{
private:
	// Keyvalue to set
	std::string _key;

private:

	void onBrowseButtonClick() override;

public:

	// Default constructor for the map
	SkinPropertyEditor() { }

	// Main constructor
	SkinPropertyEditor(wxWindow* parent, Entity* entity,
					   const std::string& name,
					   const std::string& options);

	// Clone method for virtual construction
	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
								const std::string& name,
								const std::string& options) override
	{
		return PropertyEditorPtr(new SkinPropertyEditor(parent, entity, name, options));
	}

	std::string runDialog(Entity* entity, const std::string& key) override;
};

} // namespace
