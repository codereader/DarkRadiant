#include "KeyValueTable.h"
#include "TextColumn.h"

#include <boost/format.hpp>

namespace gtkutil
{

namespace
{

// Column setup for the list store
struct Columns: public Gtk::TreeModel::ColumnRecord
{
    Gtk::TreeModelColumn<Glib::ustring> key;
    Gtk::TreeModelColumn<Glib::ustring> value;

    Columns() { add(key); add(value); }
};

const Columns& COLUMNS()
{
    static const Columns _instance;
    return _instance;
}

}

KeyValueTable::KeyValueTable()
: _store(Gtk::ListStore::create(COLUMNS()))
{
    // Add colums to the view
    append_column(*Gtk::manage(new TextColumn("Key", COLUMNS().key)));
    append_column(*Gtk::manage(new TextColumn("Value", COLUMNS().value)));

    set_headers_visible(false);
    set_model(_store);
}

void KeyValueTable::clear()
{
    _store->clear();
}

void KeyValueTable::append(const std::string& key, const std::string& value)
{
    Gtk::TreeModel::Row row = *_store->append();

    row[COLUMNS().key] = (boost::format("<b>%1%</b>") % key).str();
    row[COLUMNS().value] = value;
}

}
