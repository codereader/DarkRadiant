#pragma once

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
: public Gtk::Window
{
    // The Gtk::Builder used to load the .glade file
    Glib::RefPtr<Gtk::Builder> _builder;

public:

    /// Constructor
    GladeWindow()
    : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
    { }

    /**
     * \brief
     * Add a child widget from a Gtk::Builder object.
     *
     * \param builder
     * The Gtk::Builder containing the child widget hierarchy.
     *
     * \param childName
     * The name of the widget within the Gtk::Builder whose child should be
     * placed into this window. The childName must refer to a Gtk::Bin
     * subclass, which may contain only a single child widget.
     */
    void addChildFromBuilder(Glib::RefPtr<Gtk::Builder> builder, 
                             const std::string& childName);

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

};

}
