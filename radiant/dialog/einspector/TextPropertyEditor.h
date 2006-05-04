#ifndef TEXTPROPERTYEDITOR_H_
#define TEXTPROPERTYEDITOR_H_

#include "PropertyEditor.h"
#include "PropertyEditorRegistrationHelper.h"

namespace ui
{

/* TextPropertyEditor
 * 
 * PropertyEditor that displays and edits a simple text string (all Entity key
 * values are in fact text strings).
 */

class TextPropertyEditor:
    public PropertyEditor
{
    // Registration helper
    static PropertyEditorRegistrationHelper _helper;

public:

    virtual GtkWidget* getWidget() {}
    virtual PropertyEditor* createNew(Entity* entity, const char* name) {}
    virtual void refresh() {}
    virtual void commit() {}
    
};

// Initialise the Registration Helper
PropertyEditorRegistrationHelper TextPropertyEditor::_helper("TextPropertyEditor", new TextPropertyEditor());

}

#endif /*TEXTPROPERTYEDITOR_H_*/
