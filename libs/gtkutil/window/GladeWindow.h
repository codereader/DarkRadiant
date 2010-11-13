#pragma once

#include "../GladeWidgetPopulator.h"

#include <cassert>

#include <gtkmm/builder.h>
#include <gtkmm/window.h>

namespace gtkutil
{

/**
 * \brief
 * Convenience class allowing a Window to be populated easily from a
 * .glade UI file, and to access child widgets by name.
 */
class GladeWindow
: public Gtk::Window,
  public GladeWidgetPopulator
{
public:

    /// Constructor
    GladeWindow()
    : Gtk::Window(Gtk::WINDOW_TOPLEVEL),
      GladeWidgetPopulator(this)
    { }
};

}
