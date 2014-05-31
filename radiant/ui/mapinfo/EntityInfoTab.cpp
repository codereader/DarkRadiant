#include "EntityInfoTab.h"

#include "i18n.h"
#include "iradiant.h"
#include "icounter.h"

#include "string/convert.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ui
{

namespace
{
	const char* const TAB_NAME = N_("Entities");
	const std::string TAB_ICON("cmenu_add_entity.png");
}

EntityInfoTab::EntityInfoTab(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	// Create all the widgets
	populateTab();
}

std::string EntityInfoTab::getLabel()
{
	return _(TAB_NAME);
}

std::string EntityInfoTab::getIconName()
{
	return TAB_ICON;
}

void EntityInfoTab::populateTab()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the list store that contains the eclass => count map
	_listStore = new wxutil::TreeModel(_columns, true);

	// Create the treeview and pack two columns into it
	_treeView = wxutil::TreeView::CreateWithModel(this, _listStore);

	_treeView->AppendTextColumn(_("Entity Class"), _columns.eclass.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Count"), _columns.count.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Populate the liststore with the entity count information
    for (map::EntityBreakdown::Map::const_iterator i = _entityBreakdown.begin();
		 i != _entityBreakdown.end();
		 i++)
	{
		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.eclass] = i->first;
		row[_columns.count] = static_cast<int>(i->second);

		row.SendItemAdded();
	}

	// The table containing the primitive statistics
	wxGridSizer* table = new wxGridSizer(3, 2, 3, 6);

	wxStaticText* brushLabel = new wxStaticText(this, wxID_ANY, _("Brushes:"));
	wxStaticText* patchLabel = new wxStaticText(this, wxID_ANY, _("Patches:"));
	wxStaticText* entityLabel = new wxStaticText(this, wxID_ANY, _("Entities:"));

	brushLabel->SetMinSize(wxSize(75, -1));
	patchLabel->SetMinSize(wxSize(75, -1));
	entityLabel->SetMinSize(wxSize(75, -1));

	wxStaticText* brushCount = new wxStaticText(this, wxID_ANY, 
		string::to_string(GlobalCounters().getCounter(counterBrushes).get()));
	wxStaticText* patchCount = new wxStaticText(this, wxID_ANY, 
		string::to_string(GlobalCounters().getCounter(counterPatches).get()));
	wxStaticText* entityCount = new wxStaticText(this, wxID_ANY, 
		string::to_string(GlobalCounters().getCounter(counterEntities).get()));

	brushCount->SetFont(brushCount->GetFont().Bold());
	patchCount->SetFont(patchCount->GetFont().Bold());
	entityCount->SetFont(entityCount->GetFont().Bold());

	table->Add(brushLabel);
	table->Add(brushCount);

	table->Add(patchLabel);
	table->Add(patchCount);

	table->Add(entityLabel);
	table->Add(entityCount);

	GetSizer()->Add(_treeView, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(table, 0, wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

} // namespace ui
