#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/dataview/DeclarationTreeView.h"

#include <set>

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

    wxutil::DeclarationTreeView::Columns _columns;
	wxutil::DeclarationTreeView* _headsView;

	wxTextCtrl* _description;

	// The model preview
    wxutil::ModelPreviewPtr _preview;

	// The name of the currently selected head
	std::string _selectedHead;

public:
	AIHeadChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedHead(const std::string& headDef);

	// Get the currently selected head (is empty when nothing is selected)
	std::string getSelectedHead();

private:
	void populateHeadStore();

	void handleSelectionChanged();
	void onHeadSelectionChanged(wxDataViewEvent& ev);
    void _onItemActivated( wxDataViewEvent& ev );
};

} // namespace
