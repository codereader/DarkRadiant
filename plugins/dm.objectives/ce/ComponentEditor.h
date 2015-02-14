#pragma once

#include <memory>

class wxWindow;

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
typedef std::shared_ptr<ComponentEditor> ComponentEditorPtr;

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
	 * Return a wxWidget for packing into the parent dialog.
	 *
	 * The top-level widget returned by getWxWidget() is owned by the
	 * ComponentEditor, and will be destroyed via Destroy() when the
	 * the ComponentEditor's destructor is invoked.
	 */
	virtual wxWindow* getWidget() = 0;

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
	virtual ComponentEditorPtr create(wxWindow* parent, objectives::Component& comp) const = 0;

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

    /**
     * Component editors automatically write their values to the edited component
     * which can be counter-productive during construction time. Use this method
     * to activate the internal callbacks and allow the editor to push the values
     * to the edited component.
     */
    virtual void setActive(bool active) = 0;

};

} // namespace

} // namespace
