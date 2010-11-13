#include "GladeWidgetPopulator.h"

#include <gtkmm/bin.h>

namespace gtkutil
{

void GladeWidgetPopulator::reparentChildFromBuilder(
    const std::string& childName
)
{
    g_assert(_builder);

    Gtk::Bin* bin = NULL;
    _builder->get_widget(childName, bin);
    g_assert(bin);

    // Get the child from the bin, and add it to the target container as a child
    Gtk::Widget* child = bin->get_child();
    g_assert(child);

    child->show_all();
    child->reparent(*_container);
}

}
