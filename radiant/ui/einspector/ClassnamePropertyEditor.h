#ifndef CLASSNAMEPROPERTYEDITOR_H_
#define CLASSNAMEPROPERTYEDITOR_H_

#include "PropertyEditor.h"

#include <gtk/gtkwidget.h>
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
	// Main widget
	GtkWidget* _widget;
	
	// Keyvalue to set
	std::string _key;
	
private:

	/* GTK CALLBACKS */
	static void _onBrowseButton(GtkWidget*, ClassnamePropertyEditor*);
	
protected:
	
	// Return main widget to parent class
	GtkWidget* _getWidget() const 
    {
		return _widget;
	}
	
public:

	// Default constructor for the map
	ClassnamePropertyEditor() { }
	
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
