#ifndef SOUNDPROPERTYEDITOR_H_
#define SOUNDPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/**
 * Property editor for selecting a sound shader.
 */
class SoundPropertyEditor
: public PropertyEditor
{
public:

	// Default constructor for the map
	SoundPropertyEditor() { }
	
	// Main constructor
	SoundPropertyEditor(Entity* entity, 
					    const std::string& name,
					    const std::string& options);
					   
	// Clone method for virtual construction
	SoundPropertyEditor* createNew(Entity* entity,
							  	   const std::string& name,
							  	   const std::string& options)
	{
		return new SoundPropertyEditor(entity, name, options);
	}
	
	// Set the value to be displayed in the widgets
	void setValue(const std::string&);
	
	// Return the value set in the widgets
	const std::string getValue();
};

}

#endif /*SOUNDPROPERTYEDITOR_H_*/
