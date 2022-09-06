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
    ITargetKey::Ptr _key;

public:
    // Construct a EntityPropertyEditor with an entity and key to edit
    EntityPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    // Create a new EntityPropertyEditor
    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<EntityPropertyEditor>(parent, entities, key);
    }

private:
    void onBrowseButtonClick() override;
};

}
