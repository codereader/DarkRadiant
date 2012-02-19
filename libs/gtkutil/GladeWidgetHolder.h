#pragma once

#include "iuimanager.h"

#include <gtkmm/builder.h>

namespace gtkutil
{

/**
 * Convenience class to hold and own a Gtk::Builder, and to return named widgets
 * from among its contents.
 */
class GladeWidgetHolder
{
private:
    // The Gtk::Builder used to load the .glade file
    Glib::RefPtr<Gtk::Builder> _builder;

protected:

    /// Retrieve a widget from the .glade file by name.
    template<typename WidgetType>
    WidgetType* gladeWidget(const std::string& name)
    {
        g_assert(_builder);

        WidgetType* widget = NULL;
        _builder->get_widget(name, widget);

        g_assert(widget);
        return widget;
    }

    /// Initialise a GladeWidgetHolder with a local glade file
    GladeWidgetHolder(const std::string& localGladeFile)
    : _builder(GlobalUIManager().getGtkBuilderFromFile(localGladeFile))
    { }
};

}
