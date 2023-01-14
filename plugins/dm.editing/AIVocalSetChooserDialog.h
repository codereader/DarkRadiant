#pragma once

#include <set>

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/DeclarationTreeView.h"

#include "AIVocalSetPreview.h"

class wxTextCtrl;

namespace ui
{

class AIVocalSetChooserDialog :
	public wxutil::DialogBase
{
public:
	typedef std::set<std::string> SetList;

private:
    wxutil::DeclarationTreeView::Columns _columns;
	wxutil::DeclarationTreeView* _setView;

	wxTextCtrl* _description;

	// The name of the currently selected set
	std::string _selectedSet;

	AIVocalSetPreview* _preview;

public:
	AIVocalSetChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedVocalSet(const std::string& setName);

	// Get the currently selected set (is empty when nothing is selected)
	std::string getSelectedVocalSet();

private:
	void populateSetStore();

	// Searches all entity classes for available sets
	static void findAvailableSets();

	void handleSetSelectionChanged();
	void onSetSelectionChanged(wxDataViewEvent& ev);
    void _onItemActivated( wxDataViewEvent& ev );
};

} // namespace ui
