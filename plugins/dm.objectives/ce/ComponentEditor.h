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

/**
 * Shared pointer type for ComponentEditor subclasses.
 */
typedef boost::shared_ptr<ComponentEditor> ComponentEditorPtr;

/**
 * Interface for component editors. A component editor is a GtkWidget containing
 * additional widgets which are designed to adjust the properties for a
 * particular objective Component type, such as "KILL" or "LOCATION". The
 * ComponentsDialog will select an appropriate ComponentEditor based on the
 * chosen component type, and pack it into a dedicated area in the dialog.
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
};

}

}

#endif /*COMPONENTEDITOR_H_*/
