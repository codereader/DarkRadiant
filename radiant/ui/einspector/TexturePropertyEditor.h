#ifndef TEXTUREPROPERTYEDITOR_H_
#define TEXTUREPROPERTYEDITOR_H_

#include "PropertyEditor.h"

namespace ui
{
	
/** PropertyEditor that allows selection of a valid texture. The property
 * editor will display an editable text field along with a "browse" button
 * which will pop up a Tree View where a texture can be selected.
 */

class TexturePropertyEditor
: public PropertyEditor
{
	// Main widget
	GtkWidget* _widget;
	
	// Texture prefixes we are interested in
	const std::string _prefixes;
	
	// Entity to edit
	Entity* _entity;
	
	// Keyval to set
	std::string _key;
	
private:
	
	/* GTK CALLBACKS */
	static void callbackBrowse(GtkWidget*, TexturePropertyEditor*);

protected:
	
	// Return main widget to parent class
	GtkWidget* _getInternalWidget() {
		return _widget;
	}
	
public:

	// Construct a TexturePropertyEditor with an entity and key to edit
	TexturePropertyEditor(Entity* entity, 
						  const std::string& name, 
						  const std::string& options);
	
	// Construct a blank TexturePropertyEditor for use in the 
	// PropertyEditorFactory
	TexturePropertyEditor() {}

	// Create a new TexturePropertyEditor
    virtual PropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
    	return PropertyEditorPtr(
    		new TexturePropertyEditor(entity, name, options)
    	);
    }
    
};

}

#endif /*TEXTUREPROPERTYEDITOR_H_*/
