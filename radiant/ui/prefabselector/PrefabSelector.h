#pragma once

#include "imapresource.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/fsview/FileSystemView.h"
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
	struct Result
	{
		std::string prefabPath; // VFS path of the prefab
		bool insertAsGroup; // whether to insert the prefab as group
		bool recalculatePrefabOrigin; // whether to correct the origin of off-center prefabs
	};

private:
	wxPanel* _dialogPanel = nullptr;

	// Model preview widget
	std::unique_ptr<ui::MapPreview> _preview;

	// Main tree view with the folder hierarchy
	wxutil::FileSystemView* _treeView = nullptr;

	// The window position tracker
	wxutil::WindowPosition _position;
	wxutil::PanedPosition _panedPosition;

	// Last selected prefab, 
	std::string _lastPrefab;

	IMapResourcePtr _mapResource;

	wxTextCtrl* _description = nullptr;

    wxRadioButton* _useModPath = nullptr;
    wxRadioButton* _useCustomPath = nullptr;
    wxRadioButton* _useRecentPath = nullptr;
    wxComboBox* _recentPathSelector = nullptr;
    wxutil::PathEntry* _customPath = nullptr;
	wxCheckBox* _insertAsGroupBox = nullptr;
	wxCheckBox* _recalculatePrefabOriginBox = nullptr;

    std::list<std::string> _recentPaths;

	bool _handlingSelectionChange = false;

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

	// Return the selected prefab path
	std::string getSelectedPath();
	bool getInsertAsGroup();
	bool getRecalculatePrefabOrigin();

	void updateUsageInfo();
    void addCustomPathToRecentList();

	void onSelectionChanged(wxutil::FileSystemView::SelectionChangedEvent& ev);
    void onPrefabPathSelectionChanged();
	void onRescanPrefabs(wxCommandEvent& ev);
    void onFileViewTreePopulated();

	void onMainFrameShuttingDown();
    void _onItemActivated( wxDataViewEvent& ev );

public:
	int ShowModal() override;

	/**
	* Display the Selector instance, constructing it on first use, and
	* return the VFS path of the prefab selected by the user.
	*
	* @curPrefab: the name of the currently selected prefab the tree will browse to
	* Leave this empty to leave the treeview focus where it was when
	* the dialog was closed.
	*/
	static Result ChoosePrefab(const std::string& curPrefab = "");
};

}
