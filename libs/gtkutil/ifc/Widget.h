#pragma once

#include <gtk/gtkwidget.h>

namespace gtkutil
{

/**
 * \brief
 * Abstract base class for an object which wraps, owns or produces a GtkWidget.
 */
class Widget
{
protected:
   
   /**
    * \brief
    * Construct and/or return the GtkWidget.
    */
   virtual GtkWidget* _getWidget() const = 0;

public:

    /** 
     * \brief 
     * Return the GtkWidget for packing into the parent window.
     *
     * This may be a single GtkWidget or a container which encloses multiple
     * widgets. The widget is guaranteed to be shown when returned from this
     * method.     
     */
    GtkWidget* getWidget() const 
    {
       GtkWidget* w = _getWidget();
       gtk_widget_show_all(w);
       return w;
    }
};

}

