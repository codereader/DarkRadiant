#ifndef AI_FIND_BODY_COMPONENT_EDITOR_H_
#define AI_FIND_BODY_COMPONENT_EDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives {

namespace ce {

/**
 * ComponentEditor subclass for COMP_AI_FIND_BODY component type.
 * 
 * An COMP_AI_FIND_BODY component doesn't use specifiers.
 */
class AIFindBodyComponentEditor : 
	public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_AI_FIND_BODY().getName(), 
				ComponentEditorPtr(new AIFindBodyComponentEditor())
			);
		}
	} regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
	// Component to edit
	Component* _component;	
	
public:

	/**
	 * Construct a default AIFindBodyComponentEditor.
	 */
	AIFindBodyComponentEditor() : 
		_component(NULL)
	{}
	
	/**
	 * Construct a AIFindBodyComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	AIFindBodyComponentEditor(Component& component);
	
	/**
	 * Destructor
	 */
	~AIFindBodyComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new AIFindBodyComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

    void writeToComponent() const;
};

} // namespace ce

} // namespace objectives

#endif /* AI_FIND_BODY_COMPONENT_EDITOR_H_ */
