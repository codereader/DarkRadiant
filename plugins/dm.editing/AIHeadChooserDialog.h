#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/TreeView.h"

#include <set>
#include <map>

namespace ui
{

class AIHeadChooserDialog :
	public wxutil::DialogBase
{
public:
	typedef std::set<std::string> HeadList;

private:
	struct ListStoreColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListStoreColumns() : 
			name(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column name;
	};

	ListStoreColumns _columns;
	wxutil::TreeModel* _headStore;
	wxutil::TreeView* _headsView;

	wxTextCtrl* _description;

	// The model preview
    wxutil::ModelPreviewPtr _preview;

	// The name of the currently selected head
	std::string _selectedHead;

	static HeadList _availableHeads;

public:
	AIHeadChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedHead(const std::string& headDef);

	// Get the currently selected head (is empty when nothing is selected)
	std::string getSelectedHead();

private:
	void populateHeadStore();

	// Searches all entity classes for available heads
	static void findAvailableHeads();

	void handleSelectionChanged();
	void onHeadSelectionChanged(wxDataViewEvent& ev);
};

} // namespace ui
