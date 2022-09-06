#pragma once

#include "PropertyEditor.h"

#include <wx/event.h>
#include <string>

namespace ui
{

/**
 * PropertyEditor displaying a single browse button to allow the selection of
 * an EntityClass using the EntityClassChooser dialog.
 */
class ClassnamePropertyEditor : 
	public PropertyEditor
{
private:
	// Keyvalue to set
    ITargetKey::Ptr _key;

private:

	void _onBrowseButton(wxCommandEvent& ev);
    void _onShowDefinition(wxCommandEvent& ev);

public:

    // Main constructor
    ClassnamePropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<ClassnamePropertyEditor>(parent, entities, key);
    }
};

}
