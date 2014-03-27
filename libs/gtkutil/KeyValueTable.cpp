#include "KeyValueTable.h"

#include "i18n.h"
#include <boost/format.hpp>

namespace wxutil
{

namespace
{

// Column setup for the list store
struct Columns : 
	public wxutil::TreeModel::ColumnRecord
{
	Columns() :
		key(add(wxutil::TreeModel::Column::String)),
		value(add(wxutil::TreeModel::Column::String))
	{}

	wxutil::TreeModel::Column key;
	wxutil::TreeModel::Column value;
};

const Columns& COLUMNS()
{
    static const Columns _instance;
    return _instance;
}

}

KeyValueTable::KeyValueTable(wxWindow* parent) : 
	wxutil::TreeView(parent, wxDV_NO_HEADER | wxDV_SINGLE),
	_store(new wxutil::TreeModel(COLUMNS(), true)) // list model
{
	AssociateModel(_store);
	_store->DecRef();

	EnableAutoColumnWidthFix(false); // we don't need this

	// Single visible column, containing the directory/shader name and the icon
	AppendTextColumn(_("Key"), COLUMNS().key.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

	AppendTextColumn(_("Value"), COLUMNS().value.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
}

void KeyValueTable::Clear()
{
    _store->Clear();
}

void KeyValueTable::Append(const std::string& key, const std::string& value)
{
    wxutil::TreeModel::Row row = _store->AddItem();

    row[COLUMNS().key] = (boost::format("<b>%1%</b>") % key).str();
    row[COLUMNS().value] = value;

	_store->ItemAdded(_store->GetRoot(), row.getItem());
}

} // namespace

#include "TextColumn.h"

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
