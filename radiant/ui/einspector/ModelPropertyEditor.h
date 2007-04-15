#ifndef MODELPROPERTYEDITOR_H_
#define MODELPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/**
 * Property editor for "model" keys. Displays a browse button that can be used
 * to select a model using the Model Selector.
 */
class ModelPropertyEditor
: public PropertyEditor
{
	// Main widget
	GtkWidget* _widget;
	
	// Entity to edit
	Entity* _entity;
	
	// Keyvalue to set
	std::string _key;
	
private:

	/* GTK CALLBACKS */
	static void _onBrowseButton(GtkWidget*, ModelPropertyEditor*);
	
public:

	// Default constructor for the map
	ModelPropertyEditor() { }
	
	// Main constructor
	ModelPropertyEditor(Entity* entity, 
					    const std::string& name,
					    const std::string& options);
					   
	// Clone method for virtual construction
	PropertyEditorPtr createNew(Entity* entity,
								const std::string& name,
							  	const std::string& options)
	{
		return PropertyEditorPtr(
			new ModelPropertyEditor(entity, name, options)
		);
	}
	
	/**
	 * Virtual destructor.
	 */
	virtual ~ModelPropertyEditor() {
		gtk_widget_destroy(_widget);
	}
	
	/**
	 * Return the main widget.
	 */
	GtkWidget* getWidget() {
		gtk_widget_show_all(_widget);
		return _widget;
	}
	};

}

#endif /*MODELPROPERTYEDITOR_H_*/
