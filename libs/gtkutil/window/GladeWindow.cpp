#include "GladeWindow.h"

#include <cassert>

namespace gtkutil
{

void GladeWindow::addChildFromBuilder(
    Glib::RefPtr<Gtk::Builder> builder, const std::string& childName
)
{
    Gtk::Bin* bin = NULL;
    builder->get_widget(childName, bin);
    assert(bin);

    // Store the builder for derived classes to use
    _builder = builder;

    // Get the child from the bin, and add it to ourselves as a child
    Gtk::Widget* child = bin->get_child();
    assert(child);

    child->show_all();
    child->reparent(*this);
}

}
