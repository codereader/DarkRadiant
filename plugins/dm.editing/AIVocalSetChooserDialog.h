#pragma once

#include "wxutil/dialog/DialogBase.h"
#include <set>
#include <map>
#include "wxutil/TreeView.h"

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
	struct ListStoreColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListStoreColumns() : 
			name(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column name;
	};

	ListStoreColumns _columns;

	wxutil::TreeModel* _setStore;
	wxutil::TreeView* _setView;

	wxTextCtrl* _description;

	// The name of the currently selected set
	std::string _selectedSet;

	static SetList _availableSets;

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
};

} // namespace ui
