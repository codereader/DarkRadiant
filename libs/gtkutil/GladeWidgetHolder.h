#pragma once

#include <gtkmm/builder.h>

namespace gtkutil
{

/**
 * Convenience class to hold and own a Gtk::Builder, and to return named widgets
 * from among its contents.
 */
class GladeWidgetHolder
{
    // The Gtk::Builder used to load the .glade file
    Glib::RefPtr<Gtk::Builder> _builder;

protected:

    /**
     * \brief
     * Retrieve a widget from the .glade file by name.
     */
    template<typename WidgetType>
    WidgetType* getGladeWidget(const std::string& name)
    {
        g_assert(_builder);

        WidgetType* widget = NULL;
        _builder->get_widget(name, widget);

        g_assert(widget);
        return widget;
    }

    /**
     * Initialise a GladeWidgetHolder with the given Gtk::Builder.
     */
    GladeWidgetHolder(Glib::RefPtr<Gtk::Builder> builder)
    : _builder(builder)
    { }

    /// Set the builder to use.
    void setBuilder(Glib::RefPtr<Gtk::Builder> builder)
    {
        _builder = builder;
    }
};

}
