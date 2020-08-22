#include "LayerInfoTab.h"

#include "ilayer.h"
#include "i18n.h"
#include "iradiant.h"
#include "icounter.h"
#include "imap.h"

#include "string/convert.h"

#include <wx/sizer.h>
#include <wx/stattext.h>

#include "scene/LayerUsageBreakdown.h"

namespace ui
{

namespace
{
	const char* const TAB_NAME = N_("Layers");
	const std::string TAB_ICON("layers.png");
}

LayerInfoTab::LayerInfoTab(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	// Create all the widgets
	populateTab();
}

std::string LayerInfoTab::getLabel()
{
	return _(TAB_NAME);
}

std::string LayerInfoTab::getIconName()
{
	return TAB_ICON;
}

void LayerInfoTab::populateTab()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Create the list store that contains the eclass => count map
	_listStore = new wxutil::TreeModel(_columns, true);

	// Create the treeview and pack two columns into it
	_treeView = wxutil::TreeView::CreateWithModel(this, _listStore);

	_treeView->AppendTextColumn(_("Layer"), _columns.layerName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_treeView->AppendTextColumn(_("Node Count"), _columns.nodeCount.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	
	GetSizer()->Add(_treeView, 1, wxEXPAND | wxALL, 12);

	if (!GlobalMapModule().getRoot())
	{
		return; // stop here if we don't have a map loaded
	}

	// Calculate the node histogram
	scene::LayerUsageBreakdown bd = scene::LayerUsageBreakdown::CreateFromScene(true);

	// Populate the liststore with the layer-to-nodecount information
	GlobalMapModule().getRoot()->getLayerManager().foreachLayer([&](int layerId, const std::string& layerName)
	{
		if (layerId >= static_cast<int>(bd.size())) return;

		wxutil::TreeModel::Row row = _listStore->AddItem();

		row[_columns.layerName] = layerName;
		row[_columns.nodeCount] = static_cast<int>(bd[layerId]);

		row.SendItemAdded();
	});
}

} // namespace ui
