#ifndef CUSTOM_CLOCKED_COMPONENT_EDITOR_H_
#define CUSTOM_CLOCKED_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_CUSTOM_CLOCKED component type.
 * 
 * An COMP_CUSTOM_CLOCKED component requires no specifiers, just one arguments
 * and a special spawnarg.
 */
class CustomClockedComponentEditor : 
	public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_CUSTOM_CLOCKED().getName(), 
				ComponentEditorPtr(new CustomClockedComponentEditor())
			);
		}
	} regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
	// Component to edit
	Component* _component;
	
public:

	/**
	 * Construct a default CustomClockedComponentEditor.
	 */
	CustomClockedComponentEditor() : 
		_component(NULL)
	{}
	
	/**
	 * Construct an CustomClockedComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	CustomClockedComponentEditor(Component& component);
	
	/**
	 * Destructor
	 */
	~CustomClockedComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new CustomClockedComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* CUSTOM_CLOCKED_COMPONENT_EDITOR_H_ */
