#pragma once

#include "PropertyEditor.h"

#include <wx/event.h>
#include <string>

namespace ui
{

/**
 * PropertyEditor displaying a single "Show Definition".
 */
class InheritPropertyEditor :
	public PropertyEditor
{
private:
	// Keyvalue to set
    ITargetKey::Ptr _key;

private:

    void _onShowDefinition(wxCommandEvent& ev);
    void _onShowInDefTree(wxCommandEvent& ev);

public:
    InheritPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<InheritPropertyEditor>(parent, entities, key);
    }
};

}
