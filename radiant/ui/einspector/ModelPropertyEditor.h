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
	
	// Keyvalue to set
	std::string _key;
	
private:

	/* GTK CALLBACKS */
	static void _onModelButton(GtkWidget*, ModelPropertyEditor*);
	static void _onParticleButton(GtkWidget*, ModelPropertyEditor*);
	
protected:
	
	// Return main widget to parent class
	GtkWidget* _getWidget() const 
    {
		return _widget;
	}
	
public:

	// Default constructor for the map
	ModelPropertyEditor();
	
	// Main constructor
	ModelPropertyEditor(Entity* entity, 
					    const std::string& name,
					    const std::string& options);
					   
	// Clone method for virtual construction
	IPropertyEditorPtr createNew(Entity* entity,
								const std::string& name,
							  	const std::string& options)
	{
		return PropertyEditorPtr(
			new ModelPropertyEditor(entity, name, options)
		);
	}
	
};

} // namespace ui

#endif /*MODELPROPERTYEDITOR_H_*/
