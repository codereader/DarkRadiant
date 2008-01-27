#ifndef COMPONENTEDITOR_H_
#define COMPONENTEDITOR_H_

#include <boost/shared_ptr.hpp>
#include <gtk/gtkwidget.h>

namespace objectives
{

namespace ce
{

/**
 * \namespace objectives::ce
 * ComponentEditor subclasses for the ComponentsDialog.
 */

/* FORWARD DECLS */
class ComponentEditor;
class Component;

/**
 * Shared pointer type for ComponentEditor subclasses.
 */
typedef boost::shared_ptr<ComponentEditor> ComponentEditorPtr;

/**
 * Interface for component editors. 
 * 
 * A component editor is a GtkWidget containing additional widgets which are 
 * designed to adjust the properties for a particular objective Component type,
 * such as "KILL" or "LOCATION". The ComponentsDialog will select an appropriate
 * ComponentEditor based on the chosen component type, and pack it into a 
 * dedicated area in the dialog.
 */
class ComponentEditor
{
public:
	
	/**
	 * Return a GtkWidget* for packing into the parent dialog.
	 */
	virtual GtkWidget* getWidget() const = 0;
	
	/**
	 * Create another ComponentEditor of the same subclass type as this one.
	 * This is used for virtual construction by the ComponentEditorFactory.
	 * 
	 * @return
	 * Shared pointer to a ComponentEditor of the same type as this one.
	 */
	virtual ComponentEditorPtr clone() const = 0;
	
	/**
	 * Set the Component object that this ComponentEditor will edit.
	 *
	 * This method provides a pointer to the actual Component object that
	 * the ComponentEditor needs to edit. This will also instruct the
	 * ComponentEditor to update its GTK widgets with values from the existing
	 * Component.
	 * 
	 * @param component
	 * A pointer to the Component to be edited.
	 */
	virtual void setComponent(Component* component) = 0;
};

}

}

#endif /*COMPONENTEDITOR_H_*/
