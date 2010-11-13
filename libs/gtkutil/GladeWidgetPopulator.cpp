#include "GladeWidgetPopulator.h"

#include <gtkmm/bin.h>

namespace gtkutil
{

void GladeWidgetPopulator::addChildFromBuilder(
    Glib::RefPtr<Gtk::Builder> builder, const std::string& childName
)
{
    g_assert(builder);

    Gtk::Bin* bin = NULL;
    builder->get_widget(childName, bin);
    g_assert(bin);

    // Store the builder for derived classes to use
    _builder = builder;

    // Get the child from the bin, and add it to the target container as a child
    Gtk::Widget* child = bin->get_child();
    g_assert(child);

    child->show_all();
    child->reparent(*_container);
}

}
