#include "ModelSelector.h"
#include "ModelFileFunctor.h"
#include "ModelDataInserter.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TextColumn.h"

#include "registry/bind.h"
#include "math/Vector3.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "iregistry.h"
#include "imainframe.h"
#include "imodel.h"
#include "i18n.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>

#include <gtkmm.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

namespace ui
{

// CONSTANTS
namespace
{
    const char* const MODELSELECTOR_TITLE = N_("Choose Model");

    const std::string RKEY_BASE = "user/ui/modelSelector/";
    const std::string RKEY_SPLIT_POS = RKEY_BASE + "splitPos";
    const std::string RKEY_INFO_EXPANDED = RKEY_BASE + "infoPanelExpanded";
}

// Constructor.

ModelSelector::ModelSelector()
: gtkutil::BlockingTransientWindow(
    _(MODELSELECTOR_TITLE), GlobalMainFrame().getTopLevelWindow()
  ),
  gtkutil::GladeWidgetHolder("ModelSelector.glade"),
  _modelPreview(new gtkutil::ModelPreview()),
  _treeStore(Gtk::TreeStore::create(_columns)),
  _treeStoreWithSkins(Gtk::TreeStore::create(_columns)),
  _materialsList(_modelPreview->getRenderSystem()),
  _lastModel(""),
  _lastSkin(""),
  _populated(false),
  _showOptions(true)
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

    // Set the tree store's sort behaviour
    gtkutil::TreeModel::applyFoldersFirstSortFunc(
        _treeStore, _columns.filename, _columns.isFolder
    );
    gtkutil::TreeModel::applyFoldersFirstSortFunc(
        _treeStoreWithSkins, _columns.filename, _columns.isFolder
    );

    // Set the default size of the window
    _position.connect(this);
    _position.readPosition();
    _position.fitToScreen(0.8f, 0.8f);
    _position.applyPosition();

    // The model preview is half the width and 20% of the parent's height (to
    // allow vertical shrinking)
    _modelPreview->setSize(static_cast<int>(_position.getSize()[0]*0.4f),
                           static_cast<int>(_position.getSize()[1]*0.2f));
    Gtk::Paned* splitter = gladeWidget<Gtk::Paned>("splitter");
    splitter->pack2(*_modelPreview, true, true);

    // Re-center the window
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

    // Set up view widgets
    setupTreeView();
    setupAdvancedPanel();

    // Connect buttons
    gladeWidget<Gtk::Button>("okButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ModelSelector::callbackOK)
    );
    gladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ModelSelector::callbackCancel)
    );

    // Add main box to window
    Gtk::Widget* mainBox = gladeWidget<Gtk::Widget>("main");
    add(*mainBox);

    // Store split position in registry
    registry::bindPropertyToKey(splitter->property_position(), RKEY_SPLIT_POS);
}

void ModelSelector::setupAdvancedPanel()
{
    // Create info panel and materials list
    Gtk::ScrolledWindow* infoScrolledWin = gladeWidget<Gtk::ScrolledWindow>(
        "infoScrolledWin"
    );
    infoScrolledWin->add(_infoTable);
    Gtk::ScrolledWindow* materialsScrolledWin = gladeWidget<Gtk::ScrolledWindow>(
        "materialsScrolledWin"
    );
    materialsScrolledWin->add(_materialsList);

    // Refresh preview when material visibility changed
    _materialsList.signal_visibilityChanged().connect(
        sigc::mem_fun(*_modelPreview, &Gtk::Widget::queue_draw)
    );

    // Set scroll bar policies (default in Glade is automatic but it doesn't
    // seem to take effect)
    gladeWidget<Gtk::ScrolledWindow>("topScrolledWin")->set_policy(
        Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC
    );
    infoScrolledWin->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
    materialsScrolledWin->set_policy(Gtk::POLICY_AUTOMATIC,
                                     Gtk::POLICY_AUTOMATIC);

    // Persistent expander position
    registry::bindPropertyToKey(
        gladeWidget<Gtk::Expander>("infoExpander")->property_expanded(),
        RKEY_INFO_EXPANDED
    );
}

void ModelSelector::_onDeleteEvent()
{
    hide(); // just hide, don't call base class which might delete this dialog
}

ModelSelector& ModelSelector::Instance()
{
    ModelSelectorPtr& instancePtr = InstancePtr();

    if (instancePtr == NULL)
    {
        // Not yet instantiated, do it now
        instancePtr.reset(new ModelSelector);

        // Register this instance with GlobalRadiant() at once
        GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &ModelSelector::onRadiantShutdown)
        );
    }

    return *instancePtr;
}

ModelSelectorPtr& ModelSelector::InstancePtr()
{
    static ModelSelectorPtr _instancePtr;
    return _instancePtr;
}

void ModelSelector::onRadiantShutdown()
{
    rMessage() << "ModelSelector shutting down.\n";

    _modelPreview.reset();

    // Last step: reset the shared_ptr, triggers destruction of this instance
    InstancePtr().reset();
}

void ModelSelector::_postShow()
{
    // Conditionally hide the options
    if (!_showOptions)
    {
        gladeWidget<Gtk::Widget>("optionsBox")->hide();
    }

    // Initialise the GL widget after the widgets have been shown
    _modelPreview->initialisePreview();

    // Call the base class, will enter main loop
    BlockingTransientWindow::_postShow();
}

// Show the dialog and enter recursive main loop
ModelSelectorResult ModelSelector::showAndBlock(const std::string& curModel,
                                                bool showOptions,
                                                bool showSkins)
{
    if (!_populated)
    {
        // Attempt to construct the static instance. This could throw an
        // exception if the population of models is aborted by the user.
        try
        {
            // Populate the tree of models
            populateModels();
        }
        catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
        {
            // Return a blank model and skin
            return ModelSelectorResult("", "", false);
        }
    }

    // Choose the model based on the "showSkins" setting
    _treeView->set_model(showSkins ? _treeStoreWithSkins : _treeStore);

    // If an empty string was passed for the current model, use the last selected one
    std::string previouslySelected = (!curModel.empty()) ? curModel : _lastModel;

    if (!previouslySelected.empty())
    {
        // Lookup the model path in the treemodel
        gtkutil::TreeModel::findAndSelectString(
            _treeView,
            previouslySelected,
            _columns.vfspath
        );
    }

    showInfoForSelectedModel();

    _showOptions = showOptions;

    // show and enter recursive main loop. This will block until the dialog is hidden in some way.
    show();

	// Remove the model from the preview's scenegraph before returning
	_modelPreview->setModel("");

    // Construct the model/skin combo and return it
    return ModelSelectorResult(
        _lastModel,
        _lastSkin,
        gladeWidget<Gtk::CheckButton>("monsterClipCheckbox")->get_active()
    );
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

void ModelSelector::refresh()
{
    // Clear the flag, this triggers a new population next time the dialog is shown
    Instance()._populated = false;
}

// Helper function to create the TreeView
void ModelSelector::setupTreeView()
{
    _treeView = gladeWidget<Gtk::TreeView>("modelTreeView");

    // Single visible column, containing the directory/model name and the icon
    _treeView->append_column(*Gtk::manage(
        new gtkutil::IconTextColumn(_("Model Path"),
                                    _columns.filename,
                                    _columns.icon)
    ));

    // Use the TreeModel's full string search function
    _treeView->set_search_equal_func(
        sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains)
    );

    // Get the selection object and connect to its changed signal
    _selection = _treeView->get_selection();
    _selection->signal_changed().connect(
        sigc::mem_fun(*this, &ModelSelector::showInfoForSelectedModel)
    );
}

// Populate the tree view with models
void ModelSelector::populateModels()
{
    // Clear the treestore first
    _treeStore->clear();
    _treeStoreWithSkins->clear();

    // Create a VFSTreePopulator for the treestore
    gtkutil::VFSTreePopulator pop(_treeStore);
    gtkutil::VFSTreePopulator popSkins(_treeStoreWithSkins);

    // Use a ModelFileFunctor to add paths to the populator
    ModelFileFunctor functor(pop, popSkins);
    GlobalFileSystem().forEachFile(MODELS_FOLDER,
                                   "*",
                                   functor,
                                   0);

    // Fill in the column data (TRUE = including skins)
    ModelDataInserter inserterSkins(_columns, true);
    popSkins.forEachNode(inserterSkins);

    // Insert data into second model (FALSE = without skins)
    ModelDataInserter inserter(_columns, false);
    pop.forEachNode(inserter);

    // Set the flag, we're done
    _populated = true;
}

// Get the value from the selected column
std::string ModelSelector::getSelectedValue(int colNum)
{
    Gtk::TreeModel::iterator iter = _selection->get_selected();

    if (!iter) return ""; // nothing selected

    std::string str;
    iter->get_value(colNum, str);

    return str;
}

// Update the info table and model preview based on the current selection

void ModelSelector::showInfoForSelectedModel()
{
    // Prepare to populate the info table
    _infoTable.clear();

    // Get the model name, if this is blank we are looking at a directory,
    // so leave the table empty
    std::string mName = getSelectedValue(_columns.vfspath.index());
    if (mName.empty())
        return;

    // Get the skin if set
    std::string skinName = getSelectedValue(_columns.skin.index());

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
    _infoTable.append(_("Model name"), mName);
    _infoTable.append(_("Skin name"), skinName);
    _infoTable.append(_("Total vertices"), string::to_string(model.getVertexCount()));
    _infoTable.append(_("Total polys"), string::to_string(model.getPolyCount()));
    _infoTable.append(_("Material surfaces"), string::to_string(model.getSurfaceCount()));

    // Add the list of active materials
    _materialsList.clear();
    const model::StringList& matList(model.getActiveMaterials());
    std::for_each(
        matList.begin(), matList.end(),
        boost::bind(&MaterialsList::addMaterial, &_materialsList, _1)
    );
}

void ModelSelector::callbackOK()
{
    // Remember the selected model then exit from the recursive main loop
    _lastModel = getSelectedValue(_columns.vfspath.index());
    _lastSkin = getSelectedValue(_columns.skin.index());

    hide(); // break main loop
}

void ModelSelector::callbackCancel()
{
    _lastModel = "";
    _lastSkin = "";

    hide(); // break main loop
}

} // namespace ui
