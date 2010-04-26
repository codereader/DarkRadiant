#ifndef _READABLE_CLOSED_COMPONENT_EDITOR_H_
#define _READABLE_CLOSED_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_READABLE_CLOSED component type.
 * 
 * This component requires that the player closes a certain readable
 * (after having it opened before).
 */
class ReadableClosedComponentEditor : 
	public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_READABLE_CLOSED().getName(), 
				ComponentEditorPtr(new ReadableClosedComponentEditor())
			);
		}
	} regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
	// Component to edit
	Component* _component;	
	
	// SpecifierEditCombo for the readable
	SpecifierEditCombo _readableSpec;
	
public:

	/**
	 * Construct a default ReadableClosedComponentEditor.
	 */
	ReadableClosedComponentEditor() : 
		_widget(NULL),
		_component(NULL)
	{}
	
	/**
	 * Construct an ReadableClosedComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	ReadableClosedComponentEditor(Component& component);
	
	/**
	 * Destructor
	 */
	~ReadableClosedComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new ReadableClosedComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* _READABLE_CLOSED_COMPONENT_EDITOR_H_ */
