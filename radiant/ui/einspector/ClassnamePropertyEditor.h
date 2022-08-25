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
	std::string _key;

private:

	void _onBrowseButton(wxCommandEvent& ev);
    void _onShowDefinition(wxCommandEvent& ev);

public:

    // Main constructor
    ClassnamePropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& name);

    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const std::string& name)
    {
        return std::make_shared<ClassnamePropertyEditor>(parent, entities, name);
    }
};

}
