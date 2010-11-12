#ifndef _COMPONENT_EDITOR_BASE_H_
#define _COMPONENT_EDITOR_BASE_H_

#include "ComponentEditor.h"
#include <gtkmm/box.h>

namespace objectives
{

namespace ce
{

/**
 * greebo: Common base class for all component editor implementations.
 * Most component editors pack their widgets into a VBox, which is what
 * this class derives from (privately). This base class implements the required
 * getWidget() method, return "this".
 */
class ComponentEditorBase :
	public ComponentEditor,
	protected Gtk::VBox
{
public:
	ComponentEditorBase() :
		Gtk::VBox(false, 6)
	{}

	virtual Gtk::Widget* getWidget()
	{
		return this;
	}
};

}

}

#endif /* _COMPONENT_EDITOR_BASE_H_ */
