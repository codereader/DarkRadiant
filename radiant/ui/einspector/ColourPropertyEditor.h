#pragma once

#include "PropertyEditor.h"

#include <wx/event.h>
#include <string>

class wxColourPickerCtrl;
class wxColourPickerEvent;

namespace ui
{

/** PropertyEditor to allow selection of a colour via GtkColorSelection. The property editor
 * panel will display a GtkColorButton, which when clicked on automatically displays a
 * GtkColorSelectionDialog to choose a new colour.
 */

class ColourPropertyEditor : 
	public PropertyEditor
{
private:
	// The colour picker
	wxColourPickerCtrl* _colorButton;

	// Name of keyval
	std::string _key;

private:
	// Set the colour button from the given string
	void setColourButton(const std::string& value);

	// Return the string representation of the selected colour
	std::string getSelectedColour();

	// callback
	void _onColorSet(wxColourPickerEvent& ev);

public:

	/// Construct a ColourPropertyEditor with a given Entity and keyname
	ColourPropertyEditor(wxWindow* parent, Entity* entity, const std::string& name);

	/// Blank constructor for the PropertyEditorFactory
	ColourPropertyEditor();

	void updateFromEntity() override;

	/// Create a new ColourPropertyEditor
    virtual IPropertyEditorPtr createNew(wxWindow* parent, Entity* entity,
    									const std::string& name,
    									const std::string& options) override
	{
    	return PropertyEditorPtr(new ColourPropertyEditor(parent, entity, name));
    }
};

}

