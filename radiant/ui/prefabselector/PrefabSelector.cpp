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

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>

#include "PrefabPopulator.h"
#include "map/algorithm/WorldspawnArgFinder.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

namespace ui
{

// CONSTANTS
namespace
{
	const char* const PREFABSELECTOR_TITLE = N_("Choose Prefab");

	const std::string RKEY_BASE = "user/ui/prefabSelector/";
	const std::string RKEY_SPLIT_POS = RKEY_BASE + "splitPos";

	const char* const PREFAB_FOLDER = "prefabs/";
}

// Constructor.

PrefabSelector::PrefabSelector() :
	DialogBase(_(PREFABSELECTOR_TITLE)),
	_treeStore(new wxutil::TreeModel(_columns)),
	_treeView(NULL),
	_lastPrefab(""),
	_populated(false)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);

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
	vbox->Add(buttonSizer, 0, wxALIGN_RIGHT | wxTOP, 12);

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
	_panedPosition.applyPosition();

	Connect(wxutil::EV_TREEMODEL_POPULATION_FINISHED,
		TreeModelPopulationFinishedHandler(PrefabSelector::onTreeStorePopulationFinished), NULL, this);
}

int PrefabSelector::ShowModal()
{
	if (!_populated)
	{
		// Populate the tree
		populatePrefabs();
	}
	else
	{
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

	_preview->setRootNode(scene::INodePtr());

	_panedPosition.saveToPath(RKEY_SPLIT_POS);

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

std::string PrefabSelector::ChoosePrefab(const std::string& curPrefab)
{
	// Use the parameter only if it's not empty
	if (!curPrefab.empty())
	{
		Instance()._lastPrefab = curPrefab;
	}

	std::string returnValue = "";

	if (Instance().ShowModal() == wxID_OK)
	{
		returnValue = Instance().getSelectedValue(Instance()._columns.vfspath);
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

void PrefabSelector::populatePrefabs()
{
	_populated = true;

	// Clear the treestore first
	_treeStore->Clear();

	wxutil::TreeModel::Row row = _treeStore->AddItem();

	wxIcon prefabIcon;
	prefabIcon.CopyFromBitmap(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "cmenu_add_prefab.png"));

	row[_columns.filename] = wxVariant(wxDataViewIconText(_("Loading..."), prefabIcon));
	row[_columns.isFolder] = false;
	row.SendItemAdded();

	_populator.reset(new PrefabPopulator(_columns, this, PREFAB_FOLDER));

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

	_treeView->AssociateModel(_treeStore);
	_treeStore->DecRef();

	if (preselectItem.IsOk())
	{
		_treeView->Select(preselectItem);
		_treeView->EnsureVisible(preselectItem);

		handleSelectionChange();
	}
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

void PrefabSelector::handleSelectionChange()
{
	std::string prefabPath = getSelectedValue(_columns.vfspath);

	_mapResource = GlobalMapResourceManager().capture(prefabPath);

	if (_mapResource == NULL)
	{
		// NULLify the preview map root on failure
		_preview->setRootNode(scene::INodePtr());
		_description->SetValue("");
		return;
	}

	_lastPrefab = prefabPath;

	// Suppress the map loading dialog to avoid user
	// getting stuck in the "drag filename" operation
	{
		registry::ScopedKeyChanger<bool> changer(
			RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG, true
			);

		if (_mapResource->load())
		{
			// Get the node from the resource
			scene::INodePtr root = _mapResource->getNode();

			assert(root != NULL);

			// Set the new rootnode
			_preview->setRootNode(root);

			_preview->getWidget()->Refresh();
		}
		else
		{
			// Map load failed
			rWarning() << "Could not load prefab: " << prefabPath << std::endl;
		}
	}

	updateUsageInfo();
}

void PrefabSelector::onSelectionChanged(wxDataViewEvent& ev)
{
	handleSelectionChange();
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
