#ifndef _READABLE_OPENED_COMPONENT_EDITOR_H_
#define _READABLE_OPENED_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_READABLE_OPENED component type.
 * 
 * This component requires that the player opens a certain readable.
 */
class ReadableOpenedComponentEditor : 
	public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_READABLE_OPENED().getName(), 
				ComponentEditorPtr(new ReadableOpenedComponentEditor())
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
	 * Construct a default ItemComponentEditor.
	 */
	ReadableOpenedComponentEditor() : 
		_widget(NULL),
		_component(NULL)
	{}
	
	/**
	 * Construct an ItemComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	ReadableOpenedComponentEditor(Component& component);
	
	/**
	 * Destructor
	 */
	~ReadableOpenedComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new ReadableOpenedComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* _READABLE_OPENED_COMPONENT_EDITOR_H_ */
