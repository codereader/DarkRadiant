#pragma once

#include <gtkmm/builder.h>

namespace gtkutil
{

/**
 * Convenience class to hold and own a Gtk::Builder, and to populate the given
 * widget with its contents.
 */
class GladeWidgetPopulator
{
    // The Gtk::Builder used to load the .glade file
    Glib::RefPtr<Gtk::Builder> _builder;

    // The Gtk::Container to populate with children from the builder
    Gtk::Container* _container;

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

public:

    /// Initialise a GladeWidgetPopulator with the given target container.
    GladeWidgetPopulator(Gtk::Container* container)
    : _container(container)
    { }

    /**
     * Initialise a GladeWidgetPopulator, taking the named widget from the
     * builder and setting the given container pointer to point to it.
     *
     * This takes a reference to a Container pointer, so that the container
     * pointer may be NULL at the point of construction, and it will be filled
     * in with a pointer to the builder-created widget.
     */
    GladeWidgetPopulator(Gtk::Container*& container,
                         Glib::RefPtr<Gtk::Builder> builder,
                         const Glib::ustring& widgetName)
    : _builder(builder),
      _container(container)
    {
        g_assert(builder);
        builder->get_widget(widgetName, container);
        g_assert(container);
    }

    /**
     * \brief
     * Set the builder to use.
     *
     * For subclasses which cannot use the auto-populating constructor.
     */
    void setBuilder(Glib::RefPtr<Gtk::Builder> builder)
    {
        _builder = builder;
    }

    /**
     * \brief
     * Reparent a child widget from a Gtk::Builder object.
     *
     * \param childName
     * The name of the widget within the Gtk::Builder whose child should be
     * placed into the target container. The childName must refer to a Gtk::Bin
     * subclass, which may contain only a single child widget.
     */
    void reparentChildFromBuilder(const std::string& childName);

};

}
