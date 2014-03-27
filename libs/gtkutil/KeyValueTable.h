#pragma once

#include "TreeModel.h"
#include "TreeView.h"

namespace wxutil
{

/**
 * \brief
 * A treeview widget that shows simple key/value pairs
 *
 * This is a convenience widget that allows the quick construction of a table
 * listing keys and values, without having to create and populate a list store
 * manually.
 */
class KeyValueTable : 
	public TreeView
{
    // Our data store
    wxutil::TreeModel* _store;

public:
    /// Construct a KeyValueTable
    KeyValueTable(wxWindow* parent);

    /// Clear all entries in the table
    void Clear();

    /// Append a key/value pair
    void Append(const std::string& key, const std::string& value);
};

}

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
