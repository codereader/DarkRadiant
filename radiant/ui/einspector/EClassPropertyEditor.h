#ifndef ECLASSPROPERTYEDITOR_H_
#define ECLASSPROPERTYEDITOR_H_

#include "ComboBoxPropertyEditor.h"

namespace ui {

/**
 * greebo: Property editor which displays an editable combo box populated with all 
 * the available entity class names.
 */
class EClassPropertyEditor:
    public ComboBoxPropertyEditor
{
public:
    // Construct a EntityPropertyEditor with an entity and key to edit
	EClassPropertyEditor(Entity* entity, const std::string& name);
    
    // Construct a blank EntityPropertyEditor for use in the 
    // PropertyEditorFactory
	EClassPropertyEditor();

    // Create a new EntityPropertyEditor
    virtual PropertyEditorPtr createNew(Entity* entity, 
    									const std::string& name, 
    									const std::string& options) 
	{
        return PropertyEditorPtr(new EClassPropertyEditor(entity, name));
    }

private:
    // Populate the combo box by traversing the available EClasses
    void populateComboBox();
};

} // namespace ui

#endif /*ECLASSPROPERTYEDITOR_H_*/
