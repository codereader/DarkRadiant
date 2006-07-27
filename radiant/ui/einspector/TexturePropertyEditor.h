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
private:

	// The text entry field
	GtkWidget* _textEntry;
	
	// Texture prefixes we are interested in
	const std::string _prefixes;
	
	/* GTK CALLBACKS */
	
	static void callbackBrowse(GtkWidget*, TexturePropertyEditor*);

public:

	// Construct a TexturePropertyEditor with an entity and key to edit
	TexturePropertyEditor(Entity* entity, const std::string& name);
	
	// Construct a blank TexturePropertyEditor for use in the PropertyEditorFactory
	TexturePropertyEditor() {}

	// Create a new TexturePropertyEditor
    virtual PropertyEditor* createNew(Entity* entity, const std::string& name) {
    	return new TexturePropertyEditor(entity, name);
    }
    
    virtual void setValue(const std::string&);
    virtual const std::string getValue();
    
};

}

#endif /*TEXTUREPROPERTYEDITOR_H_*/
