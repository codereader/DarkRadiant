#pragma once

#include <gtk/gtkobject.h>

namespace gtkutil
{

/**
 * \brief
 * Interface for an object which owns, wraps or can produce a GtkObject.
 */
class Object
{
protected:

   /**
    * \brief
    * Return the GtkObject* to the superclass.
    */
   virtual GtkObject* _getGtkObject() const = 0;
};

}
