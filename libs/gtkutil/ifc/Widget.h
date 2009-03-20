#pragma once

#include "Object.h"

#include <gtk/gtkwidget.h>

namespace gtkutil
{

/**
 * \brief
 * Abstract base class for implementation of Object whose GtkObject is in fact a
 * GtkWidget.
 *
 * Since all GtkWidgets are GtkObjects, a class deriving from Widget does not
 * need to implement Object::_getGtkObject, since this functionality can be
 * provided automatically by Widget.
 */
class Widget
: public Object
{
protected:
   
   /**
    * \brief
    * Construct and/or return the GtkWidget from the subclass.
    */
   virtual GtkWidget* _getWidget() const = 0;

   /* Object implementation */
   GtkObject* _getGtkObject() const 
   {
      return GTK_OBJECT(_getWidget());
   }

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

