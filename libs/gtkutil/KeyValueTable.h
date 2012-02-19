#pragma once

#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>

namespace gtkutil
{

/**
 * \brief
 * A treeview widget that shows simple key/value pairs
 *
 * This is a convenience widget that allows the quick construction of a table
 * listing keys and values, without having to create and populate a list store
 * manually.
 */
class KeyValueTable: public Gtk::TreeView
{
    // Our data store
    Glib::RefPtr<Gtk::ListStore> _store;

public:

    /// Construct a KeyValueTable
    KeyValueTable();

    /// Clear all entries in the table
    void clear();

    /// Append a key/value pair
    void append(const std::string& key, const std::string& value);
};

}
