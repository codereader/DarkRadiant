#ifndef SKINPROPERTYEDITOR_H_
#define SKINPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/** Property Editor for the "skin" key of models. This contains a text entry
 * box with a browse button that displays a SkinChooser, which is a dialog that
 * allows the selection of both matching and generic skins to apply to the
 * given model.
 */

class SkinPropertyEditor
: public PropertyEditor
{
public:

	// Default constructor for the map
	SkinPropertyEditor() { }
	
	// Main constructor
	SkinPropertyEditor(Entity* entity, 
					   const std::string& name,
					   const std::string& options);
					   
	// Clone method for virtual construction
	PropertyEditor* createNew(Entity* entity,
							  const std::string& name,
							  const std::string& options)
	{
		return new SkinPropertyEditor(entity, name, options);
	}
	
	// Set the value to be displayed in the widgets
	void setValue(const std::string&);
	
	// Return the value set in the widgets
	const std::string getValue();
	
};

}

#endif /*SKINPROPERTYEDITOR_H_*/
