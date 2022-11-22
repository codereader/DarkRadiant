#include "ModelSelector.h"

#include "math/Vector3.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "iregistry.h"
#include "ieclass.h"
#include "ui/imainframe.h"
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
#include <wx/tglbtn.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/Bitmap.h"
#include "ui/UserInterfaceModule.h"
#include "registry/Widgets.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/sourceview/DeclarationSourceView.h"

namespace ui
{

// CONSTANTS
namespace
{
    constexpr const char* const MODELSELECTOR_TITLE = N_("Choose Model");
    constexpr const char* const SHOW_ECLASS_DEFINITION_TEXT = N_("Show Definition");
    constexpr const char* const SHOW_ECLASS_DEFINITION_ICON = "icon_classname.png";

    const std::string RKEY_BASE = "user/ui/modelSelector/";
    const std::string RKEY_SPLIT_POS = RKEY_BASE + "splitPos";
    const std::string RKEY_SHOW_SKINS = RKEY_BASE + "showSkinsInTree";
    const std::string RKEY_LAST_SELECTED_MODEL = RKEY_BASE + "lastSelectedModel";
}

// Constructor.

ModelSelector::ModelSelector() :
	DialogBase(_(MODELSELECTOR_TITLE)),
	_dialogPanel(loadNamedPanel(this, "ModelSelectorPanel")),
    _treeView(nullptr),
	_infoTable(nullptr),
    _materialsList(nullptr),
    _relatedEntityView(nullptr),
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
	findNamedObject<wxButton>(this, "ModelSelectorRescanFoldersButton")->Bind(wxEVT_BUTTON, &ModelSelector::onRescanFolders, this);

	Bind(wxEVT_CLOSE_WINDOW, &ModelSelector::_onDeleteEvent, this);

	FitToScreen(0.8f, 0.8f);

	wxSplitterWindow* splitter = findNamedObject<wxSplitterWindow>(this, "ModelSelectorSplitter");
	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.2f));
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	_panedPosition.connect(splitter);
	_panedPosition.loadFromPath(RKEY_SPLIT_POS);

	// Connect to the model cache event to get notified on reloads
	_modelsReloadedConn = GlobalModelCache().signal_modelsReloaded().connect(
		sigc::mem_fun(this, &ModelSelector::onSkinsOrModelsReloaded));

	_skinsReloadedConn = GlobalModelSkinCache().signal_skinsReloaded().connect(
		sigc::mem_fun(this, &ModelSelector::onSkinsOrModelsReloaded));

	_modelPreview->signal_ModelLoaded().connect(sigc::mem_fun(this, &ModelSelector::onModelLoaded));
}

void ModelSelector::setupAdvancedPanel(wxWindow* parent)
{
    auto detailsPage = findNamedPanel(parent, "ModelDetailsPanel");

	// Create info panel
	_infoTable = new wxutil::KeyValueTable(detailsPage);
	_infoTable->SetMinClientSize(wxSize(-1, 140));

    detailsPage->GetSizer()->Add(_infoTable, 1, wxEXPAND);

    auto materialsPage = findNamedPanel(parent, "ModelMaterialsPanel");

	_materialsList = new MaterialsList(materialsPage, _modelPreview->getRenderSystem());
    _materialsList->SetMinClientSize(wxSize(-1, 140));

    materialsPage->GetSizer()->Add(_materialsList, 1, wxEXPAND);

	// Refresh preview when material visibility changed
    _materialsList->signal_visibilityChanged().connect(
		sigc::mem_fun(*_modelPreview, &wxutil::ModelPreview::queueDraw)
    );

    auto entityPage = findNamedPanel(parent, "ModelRelatedEntitiesPanel");

    _relatedEntityStore = new wxutil::TreeModel(_relatedEntityColumns, true);
    _relatedEntityView = wxutil::TreeView::CreateWithModel(entityPage, _relatedEntityStore.get());

    _relatedEntityView->AppendTextColumn(_("Entity Class"), _relatedEntityColumns.eclassName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _relatedEntityView->AppendTextColumn(_("Skin"), _relatedEntityColumns.skin.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _relatedEntityView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ModelSelector::onRelatedEntitySelectionChange, this);
    _relatedEntityView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ModelSelector::onRelatedEntityActivated, this);

    _relatedEntityView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [&] (auto&)
    {
        _relatedEntityContextMenu->show(this);
    });

    // Construct the context menu
    _relatedEntityContextMenu = std::make_shared<wxutil::PopupMenu>();
    _relatedEntityContextMenu->addItem(
        new wxutil::IconTextMenuItem(_(SHOW_ECLASS_DEFINITION_TEXT), SHOW_ECLASS_DEFINITION_ICON),
        std::bind(&ModelSelector::onShowClassDefinition, this),
        [this]() { return _relatedEntityView->GetSelection().IsOk(); }
    );

    entityPage->GetSizer()->Prepend(_relatedEntityView, 1, wxEXPAND);
}

void ModelSelector::onShowClassDefinition()
{
    auto item = _relatedEntityView->GetSelection();
    if (!item.IsOk()) return;

    wxutil::TreeModel::Row row(item, *_relatedEntityStore.get());
    std::string eclass = row[_relatedEntityColumns.eclassName];

    auto view = new wxutil::DeclarationSourceView(this);
    view->setDeclaration(decl::Type::EntityDef, eclass);

    view->ShowModal();
    view->Destroy();
}

void ModelSelector::onRelatedEntitySelectionChange(wxDataViewEvent& ev)
{
    // Adjust the model preview to use the same skin
    auto item = _relatedEntityView->GetSelection();

    if (!item.IsOk()) return;

    wxutil::TreeModel::Row row(item, *_relatedEntityStore.get());

    std::string skin = row[_relatedEntityColumns.skin];
    _modelPreview->setSkin(skin);

    // Sync the selection in the model tree to match this model/skin combination
    _treeView->SetSelectedSkin(skin);
}

void ModelSelector::onRelatedEntityActivated(wxDataViewEvent& ev)
{
    auto item = _relatedEntityView->GetSelection();

    if (!item.IsOk()) return;

    wxutil::TreeModel::Row row(item, *_relatedEntityStore.get());

    std::string eclass = row[_relatedEntityColumns.eclassName];

    _result.objectKind = Result::ObjectKind::EntityClass;
    _result.name = eclass;
    _result.skin = "";
    _result.createClip = false;

    EndModal(wxID_OK);
    Hide();
}

void ModelSelector::cancelDialog()
{
	_panedPosition.saveToPath(RKEY_SPLIT_POS);
    _result = Result();

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

void ModelSelector::onTreeViewPopulationFinished(wxutil::ResourceTreeView::PopulationFinishedEvent& ev)
{
	findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Enable(true);
	findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Enable(true);
	findNamedObject<wxButton>(this, "ModelSelectorRescanFoldersButton")->Enable(true);

    // The modelDefs folder should start in collapsed state
    _treeView->CollapseModelDefsFolder();
}

// Show the dialog and enter recursive main loop
ModelSelector::Result ModelSelector::showAndBlock(const std::string& curModel,
    bool showOptions, bool showSkins)
{
    // Hide the Show Skins button if skins should not be shown for this invocation
    if (showSkins) {
        _showSkinsBtn->Show();
        _treeView->SetShowSkins(_showSkinsBtn->GetValue());
    }
    else {
        _showSkinsBtn->Hide();
        _treeView->SetShowSkins(false);
    }

    if (!curModel.empty())
    {
        _treeView->SetSelectedFullname(curModel);
    }
    else
    {
        // If no model provided, use the selection from the previous session
        _treeView->SetSelectedFullname(registry::getValue<std::string>(RKEY_LAST_SELECTED_MODEL));
    }

    _showOptions = showOptions;

    // Conditionally hide the options
    findNamedObject<wxPanel>(this, "ModelSelectorOptionsPanel")->Show(_showOptions);

    _result = Result();

    // Ensure the model selector is parented to the main frame,
    // it might have been constructed at a time when this was still null
    Reparent(GlobalMainFrame().getWxTopLevelWindow());

    // show and enter recursive main loop.
    int returnCode = ShowModal();

    // Remove the model from the preview's scenegraph before returning
    _modelPreview->setModel("");
    _infoModel.clear();
    _infoSkin.clear();

    return returnCode == wxID_OK ? _result : Result();
}

// Static function to display the instance, and return the selected model to the
// calling function
ModelSelector::Result ModelSelector::chooseModel(const std::string& curModel,
                                               bool showOptions,
                                               bool showSkins)
{
    // Use the instance to select a model.
    return Instance().showAndBlock(curModel, showOptions, showSkins);
}

void ModelSelector::onSkinsOrModelsReloaded()
{
    GetUserInterfaceModule().dispatch([this] ()
    {
        populateModels();
    });
}

void ModelSelector::onModelLoaded(const model::ModelNodePtr& modelNode)
{
    auto modelName = _modelPreview->getModel();
    auto skinName = _modelPreview->getSkin();

    if (modelName == _infoModel && skinName == _infoSkin)
    {
        return; // no change, don't clear everything
    }

    _infoModel = modelName;
    _infoSkin = modelName;

    _infoTable->Clear();
    _materialsList->clear();
    _relatedEntityStore->Clear();

	if (!modelNode)
	{
		return;
	}

	// Update the text in the info table
	const model::IModel& model = modelNode->getIModel();

	_infoTable->Append(_("Model name"), modelName);
	_infoTable->Append(_("Skin name"), skinName);
	_infoTable->Append(_("Total vertices"), string::to_string(model.getVertexCount()));
	_infoTable->Append(_("Total polys"), string::to_string(model.getPolyCount()));
	_infoTable->Append(_("Material surfaces"), string::to_string(model.getSurfaceCount()));

    // Model name doesn't have a folder, this could be a modelDef
    if (auto modelDef = GlobalEntityClassManager().findModel(modelName); modelDef)
    {
        _infoTable->Append(_("Defined in"), modelDef->getBlockSyntax().fileInfo.fullPath());
    }

    _materialsList->updateFromModel(model);

    wxDataViewItem matchingItem;

    // Find related entity classes
    GlobalEntityClassManager().forEachEntityClass([&] (const IEntityClassPtr& eclass)
    {
        auto modelKeyValue = eclass->getAttributeValue("model", true);

        if (modelKeyValue == modelName)
        {
            auto row = _relatedEntityStore->AddItem();

            auto eclassSkin = eclass->getAttributeValue("skin");

            row[_relatedEntityColumns.eclassName] = eclass->getDeclName();
            row[_relatedEntityColumns.skin] = eclassSkin;

            if (!matchingItem.IsOk() && eclassSkin == skinName)
            {
                matchingItem = row.getItem();
            }

            row.SendItemAdded();
        }
    });

    // Select the item that is matching the model/skin pair
    _relatedEntityView->Select(matchingItem);
}

wxWindow* ModelSelector::setupTreeViewToolbar(wxWindow* parent)
{
    // Set up the top treeview toolbar, including a custom button to enable/disable the showing of
    // skins in the tree.
    auto* toolbar = new wxutil::ResourceTreeViewToolbar(parent, _treeView);
    _showSkinsBtn = new wxBitmapToggleButton(toolbar, wxID_ANY,
                                             wxutil::GetLocalBitmap("skin16.png"));
    _showSkinsBtn->SetValue(true);
    _showSkinsBtn->SetToolTip(_("List model skins in the tree underneath their associated models"));
    _showSkinsBtn->Bind(wxEVT_TOGGLEBUTTON,
                       [this](auto& ev) { _treeView->SetShowSkins(ev.IsChecked()); });
    registry::bindWidget(_showSkinsBtn, RKEY_SHOW_SKINS);
    toolbar->GetRightSizer()->Add(_showSkinsBtn, wxSizerFlags().Border(wxLEFT, 6));

    return toolbar;
}

void ModelSelector::setupTreeView(wxWindow* parent)
{
    _treeView = new ModelTreeView(parent);
    _treeView->SetMinSize(wxSize(200, 200));

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ModelSelector::onSelectionChanged, this);
    _treeView->Bind(wxutil::EV_TREEVIEW_POPULATION_FINISHED,
                    &ModelSelector::onTreeViewPopulationFinished, this);

    // Pack in tree view and its toolbar
    parent->GetSizer()->Prepend(_treeView, 2, wxEXPAND);
    parent->GetSizer()->Prepend(setupTreeViewToolbar(parent), 0,
                                wxEXPAND | wxALIGN_LEFT | wxBOTTOM | wxLEFT | wxRIGHT, 6);
    parent->GetSizer()->Layout();

    // Accept the dialog on double-clicking on a model in the list
    _treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent& ev) {
        if (auto modelName = _treeView->GetSelectedModelPath(); !modelName.empty()) {
            onOK(ev);
        }
    });
}

void ModelSelector::populateModels()
{
    _treeView->Populate();
}

void ModelSelector::Populate()
{
    Instance().populateModels();
}

void ModelSelector::onSelectionChanged(wxDataViewEvent& ev)
{
    handleSelectionChange();
}

// Update the info table and model preview based on the current selection
void ModelSelector::handleSelectionChange()
{
    // Get the model name, if this is blank we are looking at a directory,
    // so leave the table empty
    auto modelName = _treeView->GetSelectedModelPath();

    if (modelName.empty()) return;

    // Get the skin if set
    auto skinName = _treeView->GetSelectedSkin();

    // Pass the model and skin to the preview widget
    _modelPreview->setModel(modelName);
    _modelPreview->setSkin(skinName);
}

void ModelSelector::onOK(wxCommandEvent& ev)
{
	_panedPosition.saveToPath(RKEY_SPLIT_POS);

    _result.objectKind = Result::ObjectKind::Model;
    _result.name = _treeView->GetSelectedModelPath();
    _result.skin = _treeView->GetSelectedSkin();
    _result.createClip = findNamedObject<wxCheckBox>(this, "ModelSelectorMonsterClipOption")->GetValue();

    // Remember the last selected model
    registry::setValue(RKEY_LAST_SELECTED_MODEL, _result.name);

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
	findNamedObject<wxButton>(this, "ModelSelectorRescanFoldersButton")->Enable(false);

	// This will fire the models reloaded signal after some time
	GlobalModelCache().refreshModels(false);
}

void ModelSelector::onReloadSkins(wxCommandEvent& ev)
{
	findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Enable(false);
	findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Enable(false);
    findNamedObject<wxButton>(this, "ModelSelectorRescanFoldersButton")->Enable(false);

	// When this is done, the skins reloaded signal is fired
	GlobalModelSkinCache().refresh();
}

void ModelSelector::onRescanFolders(wxCommandEvent& ev)
{
    findNamedObject<wxButton>(this, "ModelSelectorReloadModelsButton")->Enable(false);
    findNamedObject<wxButton>(this, "ModelSelectorReloadSkinsButton")->Enable(false);
    findNamedObject<wxButton>(this, "ModelSelectorRescanFoldersButton")->Enable(false);

    // This will fire the onTreeViewPopulationFinished event when done
    populateModels();
}

} // namespace ui
