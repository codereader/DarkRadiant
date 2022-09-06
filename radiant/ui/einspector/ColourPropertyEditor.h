#pragma once

#include "PropertyEditor.h"

#include <wx/event.h>
#include <string>

class wxColourPickerCtrl;
class wxColourPickerEvent;

namespace ui
{

/** PropertyEditor to allow selection of a colour via wxColourPickerCtrl. The property editor
 * panel will display a button, which when clicked on automatically displays a
 * dialog to choose a new colour.
 */

class ColourPropertyEditor : 
	public PropertyEditor
{
private:
	// The colour picker
	wxColourPickerCtrl* _colorButton;

	// The target key
    ITargetKey::Ptr _key;

private:
	// Set the colour button from the given string
	void setColourButton(const std::string& value);

	// Return the string representation of the selected colour
	std::string getSelectedColour();

	// callback
	void _onColorSet(wxColourPickerEvent& ev);

public:

	/// Construct a ColourPropertyEditor with a given Entity and keyname
	ColourPropertyEditor(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key);

	void updateFromEntity();

	/// Create a new ColourPropertyEditor
    static Ptr CreateNew(wxWindow* parent, IEntitySelection& entities, const ITargetKey::Ptr& key)
    {
        return std::make_shared<ColourPropertyEditor>(parent, entities, key);
    }
};

}

