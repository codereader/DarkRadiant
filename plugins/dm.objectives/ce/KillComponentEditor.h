#ifndef KILLCOMPONENTEDITOR_H_
#define KILLCOMPONENTEDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"
#include "SpecifierEditCombo.h"
#include "../ComponentType.h"

namespace objectives
{

namespace ce
{

/**
 * ComponentEditor subclass for KILL component type.
 * 
 * A KILL component requires that an AI must be killed. It can use all
 * specifiers except SPEC_GROUP.
 */
class KillComponentEditor 
: public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				objectives::ComponentType::COMP_KILL().getName(), 
				ComponentEditorPtr(new KillComponentEditor())
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
	 * Construct a default KillComponentEditor.
	 */
	KillComponentEditor()
	: _component(NULL)
	{ }
	
	/**
	 * Construct a KillComponentEditor with a Component object to display and
	 * edit.
	 * 
	 * @param component
	 * The Component to edit.
	 */
	KillComponentEditor(Component& component);
	
	/**
	 * Destructor.
	 */
	~KillComponentEditor();
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone(Component& component) const {
		return ComponentEditorPtr(new KillComponentEditor(component));
	}
	
	GtkWidget* getWidget() const; 

};

}

}

#endif /*KILLCOMPONENTEDITOR_H_*/
