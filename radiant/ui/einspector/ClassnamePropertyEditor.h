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
	public PropertyEditor,
	public wxEvtHandler
{
private:
	// Keyvalue to set
	std::string _key;

private:

	void _onBrowseButton(wxCommandEvent& ev);

public:

	// Default constructor for the map
	ClassnamePropertyEditor()
	{}

	// Main constructor
	ClassnamePropertyEditor(wxWindow* parent, Entity* entity,
					    	const std::string& name,
					    	const std::string& options);

	// Clone method for virtual construction
	IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
								const std::string& name,
								const std::string& options)
	{
		return PropertyEditorPtr(
			new ClassnamePropertyEditor(parent, entity, name, options)
		);
	}
};

}
