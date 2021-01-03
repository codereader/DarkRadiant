#include "ModelSelector.h"
#include "ModelPopulator.h"
#include "ModelDataInserter.h"

#include "math/Vector3.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "iregistry.h"
#include "imainframe.h"
#include "imodelcache.h"
#include "imodel.h"
#include "modelskin.h"
#include "imodelcache.h"
#include "i18n.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "string/convert.h"

#include <wx/button.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include "wxutil/dataview/ResourceTreeViewToolbar.h"

#include <functional>

namespace ui
{

// CONSTANTS
namespace
{
    const char* const MODELSELECTOR_TITLE = N_("Choose Model");

    const std::string RKEY_BASE = "user/ui/modelSelector/";
    const std::string RKEY_SPLIT_POS = RKEY_BASE + "splitPos";
}

// Constructor.

ModelSelector::ModelSelector() : 
	DialogBase(_(MODELSELECTOR_TITLE)),
	_dialogPanel(loadNamedPanel(this, "ModelSelectorPanel")),
    _treeView(nullptr),
	_infoTable(nullptr),
    _materialsList(nullptr),
	_lastModel(""),
	_lastSkin(""),
	_populated(false),
    _showSkins(true),
	_showOptions(true)
{
    // Set the default size of the window
    _position.connect(this);
    _position.readPosition();

	wxPanel* rightPanel = findNamedObject<wxPanel>(this, "ModelSelectorRightPanel");
	_modelPreview.reset(new wxutil::ModelPreview(rightPanel));

	rightPanel->GetSizer()->Add(_modelPreview->getWidget(), 1, wxEXPAND);

    // The model preview is half the width and 20% of the parent's height (to
    // allow vertical shrinking)
    _modelPreview->setSize(static_cast<int>(_position.getSize()[0]*0.4f),
                           static_cast<int>(_position.getSize()[1]*0.2f));
	
    wxPanel* leftPanel = findNamedObject<wxPanel>(this, "ModelSelectorLeftPanel");

    // Set up view widgets
    setupTreeView(leftPanel);
	setupAdvancedPanel(leftPanel);

    // Connect buttons
    findNamedObject<wxButton>(this, "ModelSelectorOkButton")->Bind(wxEVT_BUTTON, &ModelSelector::onOK, this);
    findNamedObject<wxButton>(this, "ModelSelectorCancelButton")->Bind(wxEVT_BUTTON, &ModelSelector::onCancel, this);
	findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Bind(wxEVT_BUTTON, &ModelSelector::onReloadModels, this);
	findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Bind(wxEVT_BUTTON, &ModelSelector::onReloadSkins, this);

	Bind(wxEVT_CLOSE_WINDOW, &ModelSelector::_onDeleteEvent, this);

	FitToScreen(0.8f, 0.8f);

	wxSplitterWindow* splitter = findNamedObject<wxSplitterWindow>(this, "ModelSelectorSplitter");
	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.2f));
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	_panedPosition.connect(splitter);
	_panedPosition.loadFromPath(RKEY_SPLIT_POS);

    Bind(wxutil::EV_TREEMODEL_POPULATION_FINISHED, &ModelSelector::onTreeStorePopulationFinished, this);

	// Connect to the model cache event to get notified on reloads
	_modelsReloadedConn = GlobalModelCache().signal_modelsReloaded().connect(
		sigc::mem_fun(this, &ModelSelector::onSkinsOrModelsReloaded));

	_skinsReloadedConn = GlobalModelSkinCache().signal_skinsReloaded().connect(
		sigc::mem_fun(this, &ModelSelector::onSkinsOrModelsReloaded));

	_modelPreview->signal_ModelLoaded().connect(sigc::mem_fun(this, &ModelSelector::onModelLoaded));
}

void ModelSelector::setupAdvancedPanel(wxWindow* parent)
{
	// Create info panel
	_infoTable = new wxutil::KeyValueTable(parent);
	_infoTable->SetMinClientSize(wxSize(-1, 140));

	_materialsList = new MaterialsList(parent, _modelPreview->getRenderSystem());
    _materialsList->SetMinClientSize(wxSize(-1, 140));

	// Refresh preview when material visibility changed
    _materialsList->signal_visibilityChanged().connect(
		sigc::mem_fun(*_modelPreview, &wxutil::ModelPreview::queueDraw)
    );

    parent->GetSizer()->Add(_materialsList, 0, wxEXPAND | wxTOP, 6);
	parent->GetSizer()->Add(_infoTable, 0, wxEXPAND | wxTOP, 6);
}

void ModelSelector::cancelDialog()
{
	_panedPosition.saveToPath(RKEY_SPLIT_POS);

    EndModal(wxID_CANCEL);
	Hide();
}

void ModelSelector::_onDeleteEvent(wxCloseEvent& ev)
{
    cancelDialog();
}

ModelSelector& ModelSelector::Instance()
{
    ModelSelectorPtr& instancePtr = InstancePtr();

    if (!instancePtr)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new ModelSelector);

        // Pre-destruction cleanup
        GlobalMainFrame().signal_MainFrameShuttingDown().connect(
            sigc::mem_fun(*instancePtr, &ModelSelector::onMainFrameShuttingDown)
        );
    }

    return *instancePtr;
}

ModelSelectorPtr& ModelSelector::InstancePtr()
{
    static ModelSelectorPtr _instancePtr;
    return _instancePtr;
}

void ModelSelector::onMainFrameShuttingDown()
{
    rMessage() << "ModelSelector shutting down." << std::endl;

	_modelsReloadedConn.disconnect();
	_skinsReloadedConn.disconnect();

    // Destroy the window
	SendDestroyEvent();
    InstancePtr().reset();
}

void ModelSelector::onTreeStorePopulationFinished(wxutil::TreeModel::PopulationFinishedEvent& ev)
{
    // TODO
	findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Enable(true);
	findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Enable(true);
}

void ModelSelector::preSelectModel()
{
    // If an empty string was passed for the current model, use the last selected one
    std::string previouslySelected = (!_preselectedModel.empty()) ? _preselectedModel : _lastModel;

    if (!previouslySelected.empty())
    {
        wxutil::TreeModel* model = static_cast<wxutil::TreeModel*>(_treeView->GetModel());

        // Lookup the model path in the treemodel
        wxDataViewItem found = model->FindString(previouslySelected, _columns.fullName);

        if (found.IsOk())
        {
            _treeView->Select(found);
            _treeView->EnsureVisible(found);

            showInfoForSelectedModel();
        }
    }
}

// Show the dialog and enter recursive main loop
ModelSelectorResult ModelSelector::showAndBlock(const std::string& curModel,
                                                bool showOptions,
                                                bool showSkins)
{
    _showSkins = showSkins;
    _preselectedModel = curModel;

    if (!_populated)
    {
        // Populate the tree of models
        populateModels();
    }

#if !defined(__linux__)
    // Trigger a rebuild of the tree
    _treeView->Rebuild();
#endif

    if (_populated)
    {
        preSelectModel();
    }

    showInfoForSelectedModel();

    _showOptions = showOptions;

	// Conditionally hide the options
	findNamedObject<wxPanel>(this, "ModelSelectorOptionsPanel")->Show(_showOptions);

    // show and enter recursive main loop.
    int returnCode = ShowModal();

	// Remove the model from the preview's scenegraph before returning
	_modelPreview->setModel("");

    if (returnCode == wxID_OK)
    {
        // Construct the model/skin combo and return it
        return ModelSelectorResult(
            _lastModel,
            _lastSkin,
            findNamedObject<wxCheckBox>(this, "ModelSelectorMonsterClipOption")->GetValue()
        );
    }
    else
    {
        // Return empty result on cancel
        return ModelSelectorResult("", "", false);
    }
}

// Static function to display the instance, and return the selected model to the
// calling function
ModelSelectorResult ModelSelector::chooseModel(const std::string& curModel,
                                               bool showOptions,
                                               bool showSkins)
{
    // Use the instance to select a model.
    return Instance().showAndBlock(curModel, showOptions, showSkins);
}

void ModelSelector::onIdleReloadTree(wxIdleEvent& ev)
{
	Disconnect(wxEVT_IDLE, wxIdleEventHandler(ModelSelector::onIdleReloadTree), nullptr, this);

	populateModels();
}

void ModelSelector::onSkinsOrModelsReloaded()
{
    // Clear the flag, this triggers a new population next time the dialog is shown
    _populated = false;

	// In the case we're already shown, post a refresh event when the app is idle
	// We might be called from a worker thread, so don't deadlock the app
	if (IsShownOnScreen())
	{
        // TODO: Use dispatcher
		Connect(wxEVT_IDLE, wxIdleEventHandler(ModelSelector::onIdleReloadTree), nullptr, this);
	}
}

void ModelSelector::onModelLoaded(const model::ModelNodePtr& modelNode)
{
	if (!modelNode)
	{
		return;
	}

	// Update the text in the info table
	const model::IModel& model = modelNode->getIModel();

	_infoTable->Append(_("Model name"), _modelPreview->getModel());
	_infoTable->Append(_("Skin name"), _modelPreview->getSkin());
	_infoTable->Append(_("Total vertices"), string::to_string(model.getVertexCount()));
	_infoTable->Append(_("Total polys"), string::to_string(model.getPolyCount()));
	_infoTable->Append(_("Material surfaces"), string::to_string(model.getSurfaceCount()));

	// Add the list of active materials
	_materialsList->clear();

	const model::StringList& matList(model.getActiveMaterials());

	for (const auto& material : matList)
	{
		_materialsList->addMaterial(material);
	}
}

// Helper function to create the TreeView
void ModelSelector::setupTreeView(wxWindow* parent)
{
	_treeView = new ModelTreeView(parent, _columns);
    _treeView->SetMinSize(wxSize(200, 200));

	// Get selection and connect the changed callback
	_treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ModelSelector::onSelectionChanged, this);

    auto* toolbar = new wxutil::ResourceTreeViewToolbar(parent, _treeView);

	parent->GetSizer()->Prepend(_treeView, 1, wxEXPAND);
    parent->GetSizer()->Prepend(toolbar, 0, wxALIGN_LEFT | wxBOTTOM | wxLEFT | wxRIGHT, 6);
    parent->GetSizer()->Layout();
}

void ModelSelector::populateModels()
{
    // Spawn the population thread
    _populator.reset(new ModelPopulator(_columns, _treeView));
    _populator->Run();
}

void ModelSelector::Populate()
{
    Instance().populateModels();
}

void ModelSelector::onSelectionChanged(wxDataViewEvent& ev)
{
	showInfoForSelectedModel();
}

// Update the info table and model preview based on the current selection

void ModelSelector::showInfoForSelectedModel()
{
    // Prepare to populate the info table
    _infoTable->Clear();

    // Get the model name, if this is blank we are looking at a directory,
    // so leave the table empty
    std::string mName = _treeView->GetSelectedFullname();
    if (mName.empty())
        return;

    // Get the skin if set
    std::string skinName = _treeView->GetSelectedSkin();

    // Pass the model and skin to the preview widget
    _modelPreview->setModel(mName);
    _modelPreview->setSkin(skinName);
}

void ModelSelector::onOK(wxCommandEvent& ev)
{
    // Remember the selected model then exit from the recursive main loop
    _lastModel = _treeView->GetSelectedFullname();
    _lastSkin = _treeView->GetSelectedSkin();

	_panedPosition.saveToPath(RKEY_SPLIT_POS);

	EndModal(wxID_OK); // break main loop
	Hide();
}

void ModelSelector::onCancel(wxCommandEvent& ev)
{
    cancelDialog();
}

void ModelSelector::onReloadModels(wxCommandEvent& ev)
{
	findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Enable(false);
	findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Enable(false);

	// Remember the selected model before reloading
	_preselectedModel = _treeView->GetSelectedFullname();

	// This will fire the models reloaded signal after some time
	GlobalModelCache().refreshModels(false);
}

void ModelSelector::onReloadSkins(wxCommandEvent& ev)
{
	findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Enable(false);
	findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Enable(false);

	_preselectedModel = _treeView->GetSelectedFullname();

	// When this is done, the skins reloaded signal is fired
	GlobalModelSkinCache().refresh();
}

} // namespace ui
