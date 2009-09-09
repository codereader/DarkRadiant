#ifndef COLOURPROPERTYEDITOR_H_
#define COLOURPROPERTYEDITOR_H_

#include "PropertyEditor.h"

#include <string>

namespace ui
{

/** PropertyEditor to allow selection of a colour via GtkColorSelection. The property editor
 * panel will display a GtkColorButton, which when clicked on automatically displays a
 * GtkColorSelectionDialog to choose a new colour.
 */

class ColourPropertyEditor
: public PropertyEditor
{
	// Containing widget
	GtkWidget* _widget;
	
	// The GtkColorButton
	GtkWidget* _colorButton;
	
	// Name of keyval
	std::string _key;
	
private:

	// Set the colour button from the given string
	void setColourButton(const std::string& value);
	
	// Return the string representation of the selected colour
	std::string getSelectedColour();
	
	/* GTK CALLBACKS */
	static void _onColorSet(GtkWidget*, ColourPropertyEditor* self);
	
protected:
	
	// Return main widget to parent class
	GtkWidget* _getWidget() const
    {
		return _widget;
	}
	
public:

	/// Construct a ColourPropertyEditor with a given Entity and keyname
	ColourPropertyEditor(Entity* entity, const std::string& name);

	/// Blank constructor for the PropertyEditorFactory
	ColourPropertyEditor();
	
	/// Create a new ColourPropertyEditor
    virtual IPropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
    	return PropertyEditorPtr(new ColourPropertyEditor(entity, name));
    }
};

}

#endif /*COLOURPROPERTYEDITOR_H_*/
