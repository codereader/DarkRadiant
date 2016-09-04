#pragma once

#include "iuimanager.h"
#include "imapresource.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/TreeView.h"
#include "wxutil/PathEntry.h"
#include "ui/common/MapPreview.h"

#include <memory>
#include <string>
#include <wx/combobox.h>

class wxCheckBox;
class wxSizer;
class wxRadioButton;

namespace ui
{

class PrefabSelector;
typedef std::shared_ptr<PrefabSelector> PrefabSelectorPtr;

class PrefabPopulator;

/// Dialog for browsing and selecting a prefab
class PrefabSelector :
	public wxutil::DialogBase
{
public:
	// Treemodel definition
	struct TreeColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		TreeColumns() :
			filename(add(wxutil::TreeModel::Column::IconText)),
			vfspath(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column filename;	// e.g. "chair1.pfb"
		wxutil::TreeModel::Column vfspath;	// e.g. "prefabs/chair1.pfb"
		wxutil::TreeModel::Column isFolder;	// whether this is a folder
	};

	struct Result
	{
		std::string prefabPath; // VFS path of the prefab
		bool insertAsGroup; // whether to insert the prefab as group
	};

private:
	wxPanel* _dialogPanel;

	TreeColumns _columns;

	// Model preview widget
	std::unique_ptr<ui::MapPreview> _preview;

	// Tree store containing prefab names and paths 
	wxutil::TreeModel::Ptr _treeStore;

	// Main tree view with model hierarchy
	wxutil::TreeView* _treeView;

	// The window position tracker
	wxutil::WindowPosition _position;
	wxutil::PanedPosition _panedPosition;

	// Last selected prefab, 
	std::string _lastPrefab;

	// TRUE if the treeview has been populated
	bool _populated;
	std::unique_ptr<PrefabPopulator> _populator;

	IMapResourcePtr _mapResource;

	wxTextCtrl* _description;

    wxRadioButton* _useModPath;
    wxRadioButton* _useCustomPath;
    wxRadioButton* _useRecentPath;
    wxComboBox* _recentPathSelector;
    wxutil::PathEntry* _customPath;
	wxCheckBox* _insertAsGroupBox;

    std::list<std::string> _recentPaths;

private:
	// Private constructor, creates widgets
	PrefabSelector();

	// Home of the static instance
	static PrefabSelector& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static PrefabSelectorPtr& InstancePtr();

	// Helper functions to configure GUI components
	void setupTreeView(wxWindow* parent);
    void setupPathSelector(wxSizer* parentSizer);

	// Populate the tree view with prefabs
	void populatePrefabs();

    // Get the path that should be used for prefab population
    // This reflects the settings made by the user on the top of the selector window
    std::string getPrefabFolder();

    void clearPreview();

	// Return the value from the selected column, or an empty string if nothing selected
	std::string getSelectedValue(const wxutil::TreeModel::Column& col);
	bool getInsertAsGroup();

    void handleSelectionChange();
	void updateUsageInfo();
    void addCustomPathToRecentList();

	void onSelectionChanged(wxDataViewEvent& ev);
    void onPrefabPathSelectionChanged();
	void onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev);
	void onRescanPrefabs(wxCommandEvent& ev);

public:
	int ShowModal();

	/**
	* Display the Selector instance, constructing it on first use, and
	* return the VFS path of the prefab selected by the user.
	*
	* @curPrefab: the name of the currently selected prefab the tree will browse to
	* Leave this empty to leave the treeview focus where it was when
	* the dialog was closed.
	*/
	static Result ChoosePrefab(const std::string& curPrefab = "");

	void onRadiantShutdown();
};

}
