#pragma once

#include "PropertyEditor.h"

namespace ui
{

/**
 * PropertyEditor displaying a single browse button to allow the selection of
 * an FX declaration using the FxChooser dialog.
 */
class FxPropertyEditor :
    public PropertyEditor
{
private:
    // Keyvalue to set
    ITargetKey::Ptr _key;

public:
    FxPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<FxPropertyEditor>(parent, entities, key);
    }

private:
    void _onBrowseButton(wxCommandEvent& ev);
};

    }