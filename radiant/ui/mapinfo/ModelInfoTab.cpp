#include "ModelInfoTab.h"

#include "i18n.h"
#include "icommandsystem.h"
#include "string/convert.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ui
{

namespace
{
	const char* const TAB_NAME = N_("Models");
	const std::string TAB_ICON("model16green.png");
    const char* const SELECT_ITEMS = N_("Select entities using this model");
    const char* const DESELECT_ITEMS = N_("Deselect entities using this model");
}

ModelInfoTab::ModelInfoTab(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
    _popupMenu(new wxutil::PopupMenu)
{
	// Create all the widgets
	populateTab();

    _popupMenu->addItem(
        new wxMenuItem(_popupMenu.get(), wxID_ANY, _(SELECT_ITEMS)),
        std::bind(&ModelInfoTab::_onSelectItems, this, true),
        std::bind(&ModelInfoTab::_testSelectItems, this)
    );

    _popupMenu->addItem(
        new wxMenuItem(_popupMenu.get(), wxID_ANY, _(DESELECT_ITEMS)),
        std::bind(&ModelInfoTab::_onSelectItems, this, false),
        std::bind(&ModelInfoTab::_testSelectItems, this)
    );
}

std::string ModelInfoTab::getLabel()
{
	return _(TAB_NAME);
}

std::string ModelInfoTab::getIconName()
{
	return TAB_ICON;
}

void ModelInfoTab::populateTab()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the list store that contains the eclass => count map
	_listStore = new wxutil::TreeModel(_columns, true);

	// Create the treeview and pack two columns into it
	_treeView = wxutil::TreeView::CreateWithModel(this, _listStore.get());

	_treeView->AppendTextColumn(_("Model"), _columns.model.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Polys"), _columns.polycount.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Count"), _columns.modelcount.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Skins"), _columns.skincount.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _treeView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ModelInfoTab::_onContextMenu, this);

	// Populate the liststore with the entity count information
    for (auto i = _modelBreakdown.begin(); i != _modelBreakdown.end(); ++i)
	{
		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.model] = i->first;
		row[_columns.polycount] = static_cast<int>(i->second.polyCount);
		row[_columns.modelcount] = static_cast<int>(i->second.count);
		row[_columns.skincount] = static_cast<int>(i->second.skinCount.size());

		row.SendItemAdded();
	}

	// The table containing the primitive statistics
	wxGridSizer* table = new wxGridSizer(2, 2, 3, 6);

	wxStaticText* modelsLabel = new wxStaticText(this, wxID_ANY, _("Models used:"));
	wxStaticText* skinsLabel = new wxStaticText(this, wxID_ANY, _("Named Skins used:"));

	modelsLabel->SetMinSize(wxSize(100, -1));
	skinsLabel->SetMinSize(wxSize(100, -1));

	wxStaticText* modelCount = new wxStaticText(this, wxID_ANY, 
		string::to_string(_modelBreakdown.getMap().size()));
	wxStaticText* skinCount = new wxStaticText(this, wxID_ANY, 
		string::to_string(_modelBreakdown.getNumSkins()));

	modelCount->SetFont(modelCount->GetFont().Bold());
	skinCount->SetFont(skinCount->GetFont().Bold());

	table->Add(modelsLabel);
	table->Add(modelCount);

	table->Add(skinsLabel);
	table->Add(skinCount);

	GetSizer()->Add(_treeView, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(table, 0, wxBOTTOM | wxLEFT | wxRIGHT, 12);
}

void ModelInfoTab::_onContextMenu(wxDataViewEvent& ev)
{
    _popupMenu->show(_treeView);
}

void ModelInfoTab::_onSelectItems(bool select)
{
    auto item = _treeView->GetSelection();

    if (!item.IsOk()) return;

    wxutil::TreeModel::Row row(item, *_listStore);
    std::string model = row[_columns.model];

    if (select)
    {
        GlobalCommandSystem().executeCommand("SelectItemsByModel", model);
    }
    else
    {
        GlobalCommandSystem().executeCommand("DeselectItemsByModel", model);
    }
}

bool ModelInfoTab::_testSelectItems()
{
    // Return positive if there is a selection
    return _treeView->GetSelection().IsOk();
}

} // namespace ui
