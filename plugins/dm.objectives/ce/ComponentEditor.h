#ifndef COMPONENTEDITOR_H_
#define COMPONENTEDITOR_H_

#include <boost/shared_ptr.hpp>

namespace Gtk { class Widget; }

namespace objectives
{

class Component;

namespace ce
{

/**
 * \namespace objectives::ce
 * ComponentEditor subclasses for the ComponentsDialog.
 *
 * \ingroup objectives
 */

/* FORWARD DECLS */
class ComponentEditor;

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
	 * Destructor
	 */
	virtual ~ComponentEditor() {}

	/**
	 * Return a Gtk::Widget* for packing into the parent dialog.
	 *
	 * The top-level widget returned by getWidget() is owned by the
	 * ComponentEditor, and will be destroyed via gtk_widget_destroy() when the
	 * the ComponentEditor's destructor is invoked. The widget is <b>not</b>
	 * guaranteed to be visible when it is returned, and should be shown by
	 * the parent dialog using show_all().
	 */
	virtual Gtk::Widget* getWidget() = 0;

	/**
	 * Create another ComponentEditor of the same subclass type as this one.
	 * This is used for virtual construction by the ComponentEditorFactory.
	 *
	 * @param comp
	 * A reference to the Component object that the new ComponentEditor will
	 * display and edit.
	 *
	 * @return
	 * Shared pointer to a ComponentEditor of the same type as this one.
	 */
	virtual ComponentEditorPtr clone(objectives::Component& comp) const = 0;

    /**
     * Instruct the ComponentEditor to commit its changes to the Component
     * object.
     *
     * This method causes the current data contained within the
     * ComponentEditor's GTK widgets to be written out to the Component object
     * passed into the constructor. This method is intended for the owning
     * dialog (e.g. ComponentsDialog) to invoke when an Apply button is clicked
     * by the user.
     */
    virtual void writeToComponent() const = 0;

};

}

}

#endif /*COMPONENTEDITOR_H_*/
