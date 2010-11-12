#ifndef CLASSNAMEPROPERTYEDITOR_H_
#define CLASSNAMEPROPERTYEDITOR_H_

#include "PropertyEditor.h"

#include <string>

namespace ui
{

/**
 * PropertyEditor displaying a single browse button to allow the selection of
 * an EntityClass using the EntityClassChooser dialog.
 */
class ClassnamePropertyEditor
: public PropertyEditor
{
private:
	// Keyvalue to set
	std::string _key;

private:

	void _onBrowseButton();

public:

	// Default constructor for the map
	ClassnamePropertyEditor()
	{}

	// Main constructor
	ClassnamePropertyEditor(Entity* entity,
					    	const std::string& name,
					    	const std::string& options);

	// Clone method for virtual construction
	IPropertyEditorPtr createNew(Entity* entity,
								const std::string& name,
								const std::string& options)
	{
		return PropertyEditorPtr(
			new ClassnamePropertyEditor(entity, name, options)
		);
	}
};


}

#endif /*CLASSNAMEPROPERTYEDITOR_H_*/
