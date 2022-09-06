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
	public PropertyEditor
{
private:
	// Keyvalue to set
    ITargetKey::Ptr _key;

private:

	void onBrowseButtonClick() override;

public:

	// Main constructor
    SkinPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<SkinPropertyEditor>(parent, entities, key);
    }
};

class SkinChooserDialogWrapper :
    public IPropertyEditorDialog
{
public:
    std::string runDialog(Entity* entity, const std::string& key) override;
};

} // namespace
