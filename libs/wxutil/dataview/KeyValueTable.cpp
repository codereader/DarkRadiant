#include "KeyValueTable.h"

#include "i18n.h"

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
	wxutil::TreeView(parent, nullptr, wxDV_NO_HEADER | wxDV_SINGLE),
	_store(new wxutil::TreeModel(COLUMNS(), true)) // list model
{
	AssociateModel(_store.get());

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

	wxDataViewItemAttr bold;
	bold.SetBold(true);

    row[COLUMNS().key] = key;
	row[COLUMNS().key].setAttr(bold);
    row[COLUMNS().value] = value;

	row.SendItemAdded();
}

} // namespace
