#ifndef COMBOBOXPROPERTYEDITOR_H_
#define COMBOBOXPROPERTYEDITOR_H_

#include "PropertyEditor.h"

// Forward Declaration
typedef struct _GtkComboBox GtkComboBox;

namespace ui {

/**
 * greebo: This is a base class for PropertyEditors using a simple dropdown box.
 *         On SelectionChange, the combo box text is written to the 
 *         connected entity.
 */
class ComboBoxPropertyEditor:
    public PropertyEditor
{
    // Main widget
    GtkWidget* _widget;
    
protected:
	
	// The combo box
    GtkWidget* _comboBox;
    
	// Name of keyval
	std::string _key;
   
	// Construct a EntityPropertyEditor with an entity and key to edit
	ComboBoxPropertyEditor(Entity* entity, const std::string& name);
    
    // Construct a blank ComboBoxPropertyEditor for use in the 
    // PropertyEditorFactory
	ComboBoxPropertyEditor();

	// Return main widget to parent class
	GtkWidget* _getWidget() const 
    {
		return _widget;
	}

private:
	
	// GTK Callback for Apply button
    static void onApply(GtkWidget* widget, ComboBoxPropertyEditor* self);
};

} // namespace ui

#endif /*COMBOBOXPROPERTYEDITOR_H_*/
