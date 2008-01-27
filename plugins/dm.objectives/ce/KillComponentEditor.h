#ifndef KILLCOMPONENTEDITOR_H_
#define KILLCOMPONENTEDITOR_H_

#include "ComponentEditor.h"
#include "ComponentEditorFactory.h"

namespace objectives
{

namespace ce
{

/**
 * ComponentEditor subclass for KILL component type.
 */
class KillComponentEditor 
: public ComponentEditor
{
	// Registration class
	static struct RegHelper 
	{
		RegHelper() {
			ComponentEditorFactory::registerType(
				"KILL", ComponentEditorPtr(new KillComponentEditor())
			);
		}
	} regHelper;
	
	// Component to edit
	Component* _component;	
	
public:

	/**
	 * Construct a default KillComponentEditor.
	 */
	KillComponentEditor()
	: _component(NULL)
	{ }
	
	/* ComponentEditor implementation */
	
	ComponentEditorPtr clone() const {
		return ComponentEditorPtr(new KillComponentEditor());
	}
	
	GtkWidget* getWidget() const; 
	
	void setComponent(Component* component);
};

}

}

#endif /*KILLCOMPONENTEDITOR_H_*/
