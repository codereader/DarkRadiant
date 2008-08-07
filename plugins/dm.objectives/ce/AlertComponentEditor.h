#ifndef ALERT_COMPONENT_EDITOR_H_
#define ALERT_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_ALERT component type.
 * 
 * A COMP_ALERT component requires that an AI must be alerted or not alerted.
 */
class AlertComponentEditor : 
	public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_ALERT().getName(), 
				ComponentEditorPtr(new AlertComponentEditor())
			);
		}
	} regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
	// Component to edit
	Component* _component;	
	
	// SpecifierEditCombo for the kill target
	SpecifierEditCombo _targetCombo;
	
public:

	/**
	 * Construct a default AlertComponentEditor.
	 */
	AlertComponentEditor() : 
		_component(NULL)
	{}
	
	/**
	 * Construct a AlertComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	AlertComponentEditor(Component& component);
	
	/**
	 * Destructor.
	 */
	~AlertComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new AlertComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;

};

} // namespace ce

} // namespace objectives

#endif /* ALERT_COMPONENT_EDITOR_H_ */
