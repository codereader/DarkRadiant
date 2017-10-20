#include "PrefabSelector.h"

#include "ifilesystem.h"
#include "itextstream.h"
#include "i18n.h"
#include "iradiant.h"
#include "imap.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "string/convert.h"
#include "registry/registry.h"
#include "registry/Widgets.h"
#include "os/path.h"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>

#include "PrefabPopulator.h"
#include "map/algorithm/WorldspawnArgFinder.h"

#include "string/trim.h"
#include "string/case_conv.h"
#include <functional>

namespace ui
{

// CONSTANTS
namespace
{
	const char* const PREFABSELECTOR_TITLE = N_("Choose Prefab");

	const std::string RKEY_BASE = "user/ui/prefabSelector/";
	const std::string RKEY_SPLIT_POS = RKEY_BASE + "splitPos";

	const char* const PREFAB_FOLDER = "prefabs/";

    const std::string RKEY_LAST_CUSTOM_PREFAB_PATH = RKEY_BASE + "lastPrefabPath";
    const std::string RKEY_RECENT_PREFAB_PATHS = RKEY_BASE + "recentPaths";
	const std::string RKEY_INSERT_AS_GROUP = RKEY_BASE + "insertAsGroup";
}

// Constructor.

PrefabSelector::PrefabSelector() :
	DialogBase(_(PREFABSELECTOR_TITLE)),
	_treeStore(new wxutil::TreeModel(_columns)),
	_treeView(nullptr),
	_lastPrefab(""),
	_populated(false),
    _description(nullptr),
    _useModPath(nullptr),
    _useCustomPath(nullptr),
    _useRecentPath(nullptr),
    _recentPathSelector(nullptr),
    _customPath(nullptr),
	_insertAsGroupBox(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    setupPathSelector(vbox);

	wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	setupTreeView(splitter);
	
	wxPanel* previewPanel = new wxPanel(splitter, wxID_ANY);
	previewPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	_preview.reset(new ui::MapPreview(previewPanel));

	_description = new wxTextCtrl(previewPanel, wxID_ANY, "",
		wxDefaultPosition, wxDefaultSize, wxTE_LEFT | wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
	_description->SetMinClientSize(wxSize(-1, 60));

	previewPanel->GetSizer()->Add(_description, 0, wxEXPAND | wxBOTTOM, 6);
	previewPanel->GetSizer()->Add(_preview->getWidget(), 1, wxEXPAND);

	splitter->SplitVertically(_treeView, previewPanel);

	vbox->Add(splitter, 1, wxEXPAND);

	wxStdDialogButtonSizer* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
	wxButton* reloadButton = new wxButton(this, wxID_ANY, _("Rescan Prefab Folders"));
	reloadButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(PrefabSelector::onRescanPrefabs), NULL, this);

	buttonSizer->Prepend(reloadButton, 0, wxRIGHT, 32);

	_insertAsGroupBox = new wxCheckBox(this, wxID_ANY, _("Create Group out of Prefab parts"));
	registry::bindWidget(_insertAsGroupBox, RKEY_INSERT_AS_GROUP);

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(_insertAsGroupBox, 1, wxALL, 6);
	hbox->Add(buttonSizer, 0);

	vbox->Add(hbox, 0, wxEXPAND | wxTOP, 12);

	// Set the default size of the window
	_position.connect(this);
	_position.readPosition();

	// The model preview is half the width and 20% of the parent's height (to
	// allow vertical shrinking)
	_preview->setSize(static_cast<int>(_position.getSize()[0] * 0.4f),
		static_cast<int>(_position.getSize()[1] * 0.2f));

	FitToScreen(0.8f, 0.8f);

	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.2f));

	_panedPosition.connect(splitter);
	_panedPosition.loadFromPath(RKEY_SPLIT_POS);

	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED,
		TreeModelPopulationFinishedHandler(PrefabSelector::onTreeStorePopulationFinished), NULL, this);
}

void PrefabSelector::setupPathSelector(wxSizer* parentSizer)
{
    // Path selection box
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    _useModPath = new wxRadioButton(this, wxID_ANY, _("Browse mod resources"),
                                                  wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    
    _useRecentPath = new wxRadioButton(this, wxID_ANY, _("Select recently used path:"));

    _useCustomPath = new wxRadioButton(this, wxID_ANY, _("Browse custom path:"));

    // Setup recent path selector
    _recentPathSelector = new wxComboBox(this, wxID_ANY);

    _recentPathSelector->Bind(wxEVT_COMBOBOX, [&](wxCommandEvent& ev)
    {
        _useRecentPath->SetValue(true);
        onPrefabPathSelectionChanged();
    });

    // Load recent paths from registry
    xml::NodeList recentNodes = GlobalRegistry().findXPath(RKEY_RECENT_PREFAB_PATHS + "//path");

    for (const xml::Node& node : recentNodes)
    {
        std::string recentPath = node.getAttributeValue("value");

        if (recentPath.empty()) continue;

        _recentPaths.push_back(recentPath);
        _recentPathSelector->AppendString(recentPath);
    }

    _customPath = new wxutil::PathEntry(this, true);

    // Load the most recent folder into the control
    _customPath->setValue(registry::getValue<std::string>(RKEY_LAST_CUSTOM_PREFAB_PATH));

    // Connect to the changed event
    _customPath->Bind(wxutil::EV_PATH_ENTRY_CHANGED, [&](wxCommandEvent& ev)
    {
        addCustomPathToRecentList();
        onPrefabPathSelectionChanged();
    });

    hbox->Add(_useModPath, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    hbox->Add(_useRecentPath, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
    hbox->Add(_recentPathSelector, 1, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
    hbox->Add(_useCustomPath, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
    hbox->Add(_customPath, 1, wxLEFT, 6);

    parentSizer->Add(hbox, 0, wxBOTTOM | wxEXPAND, 12);

    // Wire up the signals
    _useModPath->Bind(wxEVT_RADIOBUTTON, [&] (wxCommandEvent& ev)
    {
        onPrefabPathSelectionChanged();
    });

    _useRecentPath->Bind(wxEVT_RADIOBUTTON, [&] (wxCommandEvent& ev)
    {
        onPrefabPathSelectionChanged();
    });

    _useCustomPath->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& ev)
    {
        onPrefabPathSelectionChanged();
    });

    // Update the controls right now, this also triggers a prefab rescan
    onPrefabPathSelectionChanged();
}

void PrefabSelector::addCustomPathToRecentList()
{
    std::string customPath = string::to_lower_copy(_customPath->getValue());
    string::trim(customPath);

    if (customPath.empty())
    {
        return;
    }

    for (const auto& existing : _recentPaths)
    {
        if (existing == customPath)
        {
            return; // already in recent path list
        }
    }
    
    _recentPaths.push_back(customPath);
    _recentPathSelector->AppendString(customPath);
}

int PrefabSelector::ShowModal()
{
	if (!_populated)
	{
		// Populate the tree
		populatePrefabs();
	}
	else if (!_lastPrefab.empty()) 
	{
		// #4490: Only preselect if path is not empty, wxGTK will buffer that
		// and call ExpandAncestors() on a stale wxDataViewItem in its internal idle routine

		// Preselect the item, tree is already loaded
		// Find and select the classname
		wxDataViewItem preselectItem = _treeStore->FindString(_lastPrefab, _columns.vfspath);

		if (preselectItem.IsOk())
		{
			_treeView->Select(preselectItem);
			_treeView->EnsureVisible(preselectItem);

			handleSelectionChange();
		}
	}

	// Enter the main loop
	int returnCode = DialogBase::ShowModal();

	_preview->setRootNode(scene::IMapRootNodePtr());

	_panedPosition.saveToPath(RKEY_SPLIT_POS);

    // Remember the most recently used path
    registry::setValue<std::string>(RKEY_LAST_CUSTOM_PREFAB_PATH, _customPath->getValue());

    // Persist recent paths to registry
    GlobalRegistry().deleteXPath(RKEY_RECENT_PREFAB_PATHS + "//path");

    xml::Node parentNode = GlobalRegistry().createKey(RKEY_RECENT_PREFAB_PATHS);

    for (const auto& recentPath : _recentPaths)
    {
        if (recentPath.empty()) continue;

        xml::Node pathNode = parentNode.createChild("path");
        pathNode.setAttributeValue("value", recentPath);
    }

	return returnCode;
}

PrefabSelector& PrefabSelector::Instance()
{
	PrefabSelectorPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new PrefabSelector);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
			sigc::mem_fun(*instancePtr, &PrefabSelector::onRadiantShutdown)
			);
	}

	return *instancePtr;
}

PrefabSelectorPtr& PrefabSelector::InstancePtr()
{
	static PrefabSelectorPtr _instancePtr;
	return _instancePtr;
}

void PrefabSelector::onRadiantShutdown()
{
	rMessage() << "PrefabSelector shutting down." << std::endl;

	// Destroy the window
	SendDestroyEvent();
	InstancePtr().reset();
}

PrefabSelector::Result PrefabSelector::ChoosePrefab(const std::string& curPrefab)
{
	// Use the parameter only if it's not empty
	if (!curPrefab.empty())
	{
		Instance()._lastPrefab = curPrefab;
	}

	Result returnValue;

	returnValue.insertAsGroup = false;
	returnValue.prefabPath = "";

	if (Instance().ShowModal() == wxID_OK)
	{
		returnValue.prefabPath = Instance().getSelectedValue(Instance()._columns.vfspath);
		returnValue.insertAsGroup = Instance().getInsertAsGroup();
	}

	Instance().Hide();

	// Use the instance to select a model.
	return returnValue;
}

// Helper function to create the TreeView
void PrefabSelector::setupTreeView(wxWindow* parent)
{
	_treeView = wxutil::TreeView::CreateWithModel(parent, _treeStore, wxBORDER_STATIC | wxDV_NO_HEADER);

	// Single visible column, containing the directory/shader name and the icon
	_treeView->AppendIconTextColumn(_("Prefab"), _columns.filename.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Get selection and connect the changed callback
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(PrefabSelector::onSelectionChanged), NULL, this);

	// Use the TreeModel's full string search function
	_treeView->AddSearchColumn(_columns.filename);
}

std::string PrefabSelector::getPrefabFolder()
{
    std::string customPath = _customPath->getEntryWidget()->GetValue().ToStdString();

    // Only search in custom paths if it is absolute
    if (_useCustomPath->GetValue() && !customPath.empty() && path_is_absolute(customPath.c_str()))
    {
        return customPath;
    }
    else if (_useRecentPath->GetValue() && !_recentPathSelector->GetStringSelection().IsEmpty())
    {
        return _recentPathSelector->GetStringSelection().ToStdString();
    }
    else
    {
        // No custom path
        return PREFAB_FOLDER;
    }
}

void PrefabSelector::populatePrefabs()
{
	_populated = true;

    if (_populator && _populator->getPrefabPath() == getPrefabFolder())
    {
        // Population already running for this path
        return;
    }

    // Clear the existing run first (this waits for it to finish)
    _populator.reset();

	// Clear the treestore first
	_treeStore->Clear();

	wxutil::TreeModel::Row row = _treeStore->AddItem();

	wxIcon prefabIcon;
	prefabIcon.CopyFromBitmap(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "cmenu_add_prefab.png"));

	row[_columns.filename] = wxVariant(wxDataViewIconText(_("Loading..."), prefabIcon));
	row[_columns.isFolder] = false;
	row[_columns.vfspath] = "__loadingnode__"; // to pevent that item from being found
	row.SendItemAdded();

    _populator.reset(new PrefabPopulator(_columns, this, getPrefabFolder()));

	// Start the thread, will send an event when finished
	_populator->populate();
}

void PrefabSelector::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
	_treeStore = ev.GetTreeModel();

	wxDataViewItem preselectItem;

	if (!_lastPrefab.empty())
	{
		// Find and select the classname
		preselectItem = _treeStore->FindString(_lastPrefab, _columns.vfspath);
	}

	_treeView->AssociateModel(_treeStore.get());

	if (preselectItem.IsOk())
	{
		_treeView->Select(preselectItem);
		_treeView->EnsureVisible(preselectItem);

		handleSelectionChange();
	}

    _populator.reset();

    // Auto-size the first level
    _treeView->TriggerColumnSizeEvent();
}

void PrefabSelector::onRescanPrefabs(wxCommandEvent& ev)
{
	populatePrefabs();
}

// Get the value from the selected column
std::string PrefabSelector::getSelectedValue(const wxutil::TreeModel::Column& col)
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return "";

	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[col];
}

bool PrefabSelector::getInsertAsGroup()
{
	return _insertAsGroupBox->GetValue();
}

void PrefabSelector::clearPreview()
{
    // NULLify the preview map root on failure
    _preview->setRootNode(scene::IMapRootNodePtr());
    _preview->queueDraw();

    _description->SetValue("");
}

void PrefabSelector::handleSelectionChange()
{
    wxDataViewItem item = _treeView->GetSelection();

    if (!item.IsOk())
    {
        clearPreview();
        return;
    }

    wxutil::TreeModel::Row row(item, *_treeView->GetModel());

    if (row[_columns.isFolder].getBool())
    {
        clearPreview();
        return;
    }

    std::string prefabPath = row[_columns.vfspath];

	_mapResource = GlobalMapResourceManager().loadFromPath(prefabPath);

	if (!_mapResource)
	{
        clearPreview();
		return;
	}

	_lastPrefab = prefabPath;

	// Suppress the map loading dialog to avoid user
	// getting stuck in the "drag filename" operation
	registry::ScopedKeyChanger<bool> changer(
		RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG, true
	);

	if (_mapResource->load())
	{
		// Get the node from the resource
        scene::IMapRootNodePtr root = _mapResource->getNode();

		assert(root);

		// Set the new rootnode
		_preview->setRootNode(root);

		_preview->getWidget()->Refresh();
	}
	else
	{
		// Map load failed
		rWarning() << "Could not load prefab: " << prefabPath << std::endl;
        clearPreview();
	}

	updateUsageInfo();
}

void PrefabSelector::onSelectionChanged(wxDataViewEvent& ev)
{
	handleSelectionChange();
}

void PrefabSelector::onPrefabPathSelectionChanged()
{
    _customPath->Enable(_useCustomPath->GetValue());

    // Reload the prefabs from the newly selected path
    populatePrefabs();
}

void PrefabSelector::updateUsageInfo()
{
	std::string usage("");

	if (_mapResource != NULL && _mapResource->getNode() != NULL)
	{
		// Retrieve the root node
		scene::INodePtr root = _mapResource->getNode();

		// Traverse the root to find the worldspawn
		map::WorldspawnArgFinder finder("editor_description");
		root->traverse(finder);

		usage = finder.getFoundValue();

		if (usage.empty())
		{
			usage = _("<no description>");
		}
	}

	_description->SetValue(usage);
}

} // namespace ui
