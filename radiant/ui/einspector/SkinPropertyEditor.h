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
	
	// Main text entry
	GtkWidget* _textEntry;
	
	// Entity to edit
	Entity* _entity;
	
private:

	/* GTK CALLBACKS */
	static void _onBrowseButton(GtkWidget* w, SkinPropertyEditor* self);
	
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

	/**
	 * Return the main widget.
	 */
	GtkWidget* getWidget() {
		return _widget;
	}
		
};

}

#endif /*SKINPROPERTYEDITOR_H_*/
