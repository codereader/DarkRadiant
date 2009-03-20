#ifndef EDITORWIDGET_H_
#define EDITORWIDGET_H_

#include "Widget.h"

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
: public Widget
{
public:

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
