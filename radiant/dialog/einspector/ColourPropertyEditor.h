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
public:

	/// Construct a ColourPropertyEditor with a given Entity and keyname
	ColourPropertyEditor(Entity* entity, const std::string& name);

	/// Blank constructor for the PropertyEditorFactory
	ColourPropertyEditor();
	
	/// Create a new ColourPropertyEditor
	virtual PropertyEditor* createNew(Entity* entity, const std::string& name) {
    	return new ColourPropertyEditor(entity, name);
    }

	/// Set the displayed value to the given keyval		
    virtual void setValue(const std::string&);
    
    /// Return the currently-selected value to the parent
    virtual const std::string getValue();
};

}

#endif /*COLOURPROPERTYEDITOR_H_*/
