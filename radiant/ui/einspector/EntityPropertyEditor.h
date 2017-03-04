#pragma once

#include "PropertyEditor.h"

class wxCommandEvent;

namespace ui
{

/**
 * Property editor which displays a button opening a separate dialog, filled with the
 * entities in the map. This is used for properties like "target" and "bind" which
 * should reference a different entity.
 */
class EntityPropertyEditor:
    public PropertyEditor
{
protected:
	// Keyvalue to set
	std::string _key;

public:
    // Construct a EntityPropertyEditor with an entity and key to edit
    EntityPropertyEditor(wxWindow* parent, Entity* entity, const std::string& name);

    // Construct a blank EntityPropertyEditor for use in the
    // PropertyEditorFactory
    EntityPropertyEditor();

	void updateFromEntity() override;

    // Create a new EntityPropertyEditor
    virtual IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    									const std::string& name,
    									const std::string& options) override
	{
        return PropertyEditorPtr(new EntityPropertyEditor(parent, entity, name));
    }

private:
    void onBrowseButtonClick() override;
};

}
