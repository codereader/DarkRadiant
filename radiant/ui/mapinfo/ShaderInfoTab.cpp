#include "ShaderInfoTab.h"

#include "i18n.h"

#include "string/string.h"

#include <functional>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "selection/algorithm/Shader.h"

namespace ui
{

namespace
{
	const char* const TAB_NAME = N_("Shaders");
	const std::string TAB_ICON("icon_texture.png");
	const char* const SELECT_ITEMS = N_("Select elements using this shader");
	const char* const DESELECT_ITEMS = N_("Deselect elements using this shader");
}

ShaderInfoTab::ShaderInfoTab(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_listStore(new wxutil::TreeModel(_columns, true)),
	_treeView(wxutil::TreeView::CreateWithModel(this, _listStore)),
	_popupMenu(new wxutil::PopupMenu)
{
	// Create all the widgets
	construct();

	_popupMenu->addItem(
		new wxMenuItem(_popupMenu.get(), wxID_ANY, _(SELECT_ITEMS)),
		std::bind(&ShaderInfoTab::_onSelectItems, this, true),
		std::bind(&ShaderInfoTab::_testSelectItems, this)
	);

	_popupMenu->addItem(
		new wxMenuItem(_popupMenu.get(), wxID_ANY, _(DESELECT_ITEMS)),
		std::bind(&ShaderInfoTab::_onSelectItems, this, false),
		std::bind(&ShaderInfoTab::_testSelectItems, this)
	);
}

std::string ShaderInfoTab::getLabel()
{
	return _(TAB_NAME);
}

std::string ShaderInfoTab::getIconName()
{
	return TAB_ICON;
}

void ShaderInfoTab::construct()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	_treeView->AppendTextColumn(_("Shader"), _columns.shader.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Faces"), _columns.faceCount.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Patches"), _columns.patchCount.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Populate the liststore with the entity count information
    for (map::ShaderBreakdown::Map::const_iterator i = _shaderBreakdown.begin();
		 i != _shaderBreakdown.end();
		 ++i)
	{
		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.shader] = i->first;
		row[_columns.faceCount] = static_cast<int>(i->second.faceCount);
		row[_columns.patchCount] = static_cast<int>(i->second.patchCount);

		row.SendItemAdded();
	}

    // The table containing the statistics
	wxGridSizer* table = new wxGridSizer(1, 2, 3, 6);

	wxStaticText* shaderLabel = new wxStaticText(this, wxID_ANY, _("Shaders used:"));
	shaderLabel->SetMinSize(wxSize(100, -1));

	wxStaticText* shaderCount = new wxStaticText(this, wxID_ANY, 
		string::to_string(_shaderBreakdown.getMap().size()));

	shaderCount->SetFont(shaderCount->GetFont().Bold());

	table->Add(shaderLabel);
	table->Add(shaderCount);

	GetSizer()->Add(_treeView, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(table, 0, wxBOTTOM | wxLEFT | wxRIGHT, 12);

	_treeView->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(ShaderInfoTab::_onContextMenu), NULL, this);
}

void ShaderInfoTab::_onContextMenu(wxDataViewEvent& ev)
{
	_popupMenu->show(_treeView);
}

void ShaderInfoTab::_onSelectItems(bool select)
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return;

	wxutil::TreeModel::Row row(item, *_listStore);
	std::string shader = row[_columns.shader];

	if (select)
	{
		selection::algorithm::selectItemsByShader(shader);
	}
	else
	{
		selection::algorithm::deselectItemsByShader(shader);
	}
}

bool ShaderInfoTab::_testSelectItems()
{
	// Return positive if there is a selection
	return _treeView->GetSelection().IsOk();
}


} // namespace ui
