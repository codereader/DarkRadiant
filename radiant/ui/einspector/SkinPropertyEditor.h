#ifndef SKINPROPERTYEDITOR_H_
#define SKINPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{

/** 
 * Property Editor for the "skin" key of models. This contains a text entry box
 * with a browse button that displays a SkinChooser, which is a dialog that 
 * allows the selection of both matching and generic skins to apply to the given
 * model.
 */
class SkinPropertyEditor
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
	static void _onBrowseButton(GtkWidget* w, SkinPropertyEditor* self);
	
protected:
	
	// Return main widget to parent class
	GtkWidget* _getInternalWidget() {
		return _widget;
	}
	
public:

	// Default constructor for the map
	SkinPropertyEditor() { }
	
	// Main constructor
	SkinPropertyEditor(Entity* entity, 
					   const std::string& name,
					   const std::string& options);
					   
	// Clone method for virtual construction
	PropertyEditorPtr createNew(Entity* entity,
								const std::string& name,
								const std::string& options)
	{
		return PropertyEditorPtr(new SkinPropertyEditor(entity, name, options));
	}

};

}

#endif /*SKINPROPERTYEDITOR_H_*/
