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

#include "PrefabPopulator.h"

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
	_preview.reset(new ui::MapPreview(splitter));

	splitter->SplitVertically(_treeView, _preview->getWidget());

	vbox->Add(splitter, 1, wxEXPAND);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxTOP, 12);

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
	Instance()._lastPrefab = curPrefab;

	Instance().ShowModal();
	Instance().Hide();

	// Use the instance to select a model.
	return Instance().getSelectedValue(Instance()._columns.vfspath);
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
	/* wxTODO _treeView->set_search_equal_func(
	sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains)
	);*/
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
	}
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
		return;
	}

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
}

void PrefabSelector::onSelectionChanged(wxDataViewEvent& ev)
{
	handleSelectionChange();
}

#if 0
// Update the info table and model preview based on the current selection

void PrefabSelector::showInfoForSelectedModel()
{
	// Prepare to populate the info table
	_infoTable->Clear();

	// Get the model name, if this is blank we are looking at a directory,
	// so leave the table empty
	std::string mName = getSelectedValue(_columns.vfspath);
	if (mName.empty())
		return;

	// Get the skin if set
	std::string skinName = getSelectedValue(_columns.skin);

	// Pass the model and skin to the preview widget
	_modelPreview->setModel(mName);
	_modelPreview->setSkin(skinName);

	// Check that the model is actually valid by querying the IModelPtr
	// returned from the preview widget.
	scene::INodePtr mdl = _modelPreview->getModelNode();
	if (!mdl) {
		return; // no valid model
	}

	model::ModelNodePtr modelNode = Node_getModel(mdl);

	if (!modelNode)
	{
		return;
	}

	// Update the text in the info table
	const model::IModel& model = modelNode->getIModel();
	_infoTable->Append(_("Model name"), mName);
	_infoTable->Append(_("Skin name"), skinName);
	_infoTable->Append(_("Total vertices"), string::to_string(model.getVertexCount()));
	_infoTable->Append(_("Total polys"), string::to_string(model.getPolyCount()));
	_infoTable->Append(_("Material surfaces"), string::to_string(model.getSurfaceCount()));

	// Add the list of active materials
	_materialsList->clear();

	const model::StringList& matList(model.getActiveMaterials());

	std::for_each(
		matList.begin(), matList.end(),
		boost::bind(&MaterialsList::addMaterial, _materialsList, _1)
		);
}
#endif

} // namespace ui
