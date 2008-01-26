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
	static struct RegistrationHelper 
	{
		RegistrationHelper() {
			ComponentEditorFactory::registerType(
				"KILL", ComponentEditorPtr(new KillComponentEditor())
			);
		}
	} registrationHelper;
	
public:

	// ComponentEditor implementation
	
	ComponentEditorPtr clone() const {
		return ComponentEditorPtr(new KillComponentEditor());
	}
	
	GtkWidget* getWidget() const; 
};

}

}

#endif /*KILLCOMPONENTEDITOR_H_*/
