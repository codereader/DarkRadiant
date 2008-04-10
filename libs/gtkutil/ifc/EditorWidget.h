#ifndef EDITORWIDGET_H_
#define EDITORWIDGET_H_

#include <gtk/gtkwidget.h>
#include <string>

namespace gtkutil
{

/**
 * Interface for a custom widget which is capable of editing a string value
 * using GTK widgets appropriate to a certain value type.
 *
 * Each EditorWidget provides an interface for editing a single string which is
 * set and retrieved with the setValue() and getValue() methods. The
 * EditorWidget constructs the necessary GTK widgets and returns a single parent
 * via the getWidget() method, which may be packed into the parent dialog by the
 * calling code.
 *
 * The expectation is that a series of EditorWidgets will be created by some
 * kind of factory object, in order to select a suitable editing interface for a
 * particular set of value types.
 */
class EditorWidget
{
protected:

    /**
     * Return the actual editing widget. This method is called by the public
     * getWidget() method, which calls gtk_widget_show_all() on the returned
     * widget before in turn returning it to the calling code.
     */
    virtual GtkWidget* _getWidget() const = 0;

public:

    /**
     * Return the editor GtkWidget for packing into the parent window. This may
     * be a single GtkWidget or a container which encloses multiple widgets. The
     * widget is guaranteed to be shown when returned from this method.     *
     */
    GtkWidget* getWidget() const {
        GtkWidget* w = _getWidget();
        gtk_widget_show_all(w);
        return w;
    }

    /**
     * Set the value of the string which should be edited by this widget. The
     * child editing widgets will be immediately updated to reflect the new
     * value.
     */
    virtual void setValue(const std::string& val) = 0;

    /**
     * Get the current value of the string which is being edited by this widget.
     */
    virtual std::string getValue() const = 0;

};

}

#endif
