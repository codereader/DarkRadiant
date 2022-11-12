#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/dataview/DeclarationTreeView.h"

namespace ui
{

class AIHeadChooserDialog :
	public wxutil::DialogBase
{
private:
    wxutil::DeclarationTreeView::Columns _columns;
	wxutil::DeclarationTreeView* _headsView;

	wxTextCtrl* _description;

	// The model preview
    std::unique_ptr<wxutil::ModelPreview> _preview;

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
