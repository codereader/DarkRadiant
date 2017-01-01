#pragma once

#include "layers/LayerUsageBreakdown.h"

#include <wx/panel.h>
#include "wxutil/TreeView.h"

namespace ui
{

class LayerInfoTab :
	public wxPanel
{
private:
	// Treemodel definition
	struct ListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListColumns() :
			layerName(add(wxutil::TreeModel::Column::String)),
			nodeCount(add(wxutil::TreeModel::Column::Integer))
		{}

		wxutil::TreeModel::Column layerName;
		wxutil::TreeModel::Column nodeCount;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	wxutil::TreeModel::Ptr _listStore;
	wxutil::TreeView* _treeView;

public:
	// Constructor
	LayerInfoTab(wxWindow* parent);

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to create the widgets
	void populateTab();

};

} // namespace ui
