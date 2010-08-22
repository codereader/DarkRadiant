#include "ModelSelector.h"
#include "ModelFileFunctor.h"
#include "ModelDataInserter.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "math/Vector3.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "iregistry.h"
#include "imainframe.h"
#include "imodelpreview.h"
#include "imodel.h"
#include "i18n.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/expander.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

// CONSTANTS
namespace
{	
	const char* const MODELSELECTOR_TITLE = N_("Choose Model");
	const char* const RKEY_PREVIEW_SIZE_FACTOR = "user/ui/ModelSelector/previewSizeFactor";
}

// Constructor.

ModelSelector::ModelSelector()
: gtkutil::BlockingTransientWindow(_(MODELSELECTOR_TITLE), GlobalMainFrame().getTopLevelWindow()),
  _vbox(NULL),
  _modelPreview(GlobalUIManager().createModelPreview()),
  _treeStore(Gtk::TreeStore::create(_columns)),
  _treeStoreWithSkins(Gtk::TreeStore::create(_columns)),
  _infoStore(Gtk::ListStore::create(_infoStoreColumns)),
  _lastModel(""),
  _lastSkin(""),
  _populated(false),
  _showOptions(true)
{
	// Set the tree store's sort behaviour
	gtkutil::TreeModel::applyFoldersFirstSortFunc(_treeStore, _columns.filename, _columns.isFolder);
	gtkutil::TreeModel::applyFoldersFirstSortFunc(_treeStoreWithSkins, _columns.filename, _columns.isFolder);

	// Window properties
	set_border_width(12);

	_position.connect(this);

	// Size the model preview widget
	float previewHeightFactor = GlobalRegistry().getFloat(RKEY_PREVIEW_SIZE_FACTOR);

	// Set the default size of the window
	_position.readPosition();
	_position.fitToScreen(0.8f, previewHeightFactor);
	_position.applyPosition();

	_modelPreview->setSize(_position.getSize()[1]);

	// Re-center the window
	set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	// Main window contains a VBox. On the top goes the widgets, the bottom
	// contains the button panel
	_vbox = Gtk::manage(new Gtk::VBox(false, 6));
	
	// Pack the tree view into a VBox above the info panel
	Gtk::VBox* leftVbx = Gtk::manage(new Gtk::VBox(false, 6));
	leftVbx->pack_start(createTreeView(), true, true, 0);
	leftVbx->pack_start(createInfoPanel(), true, true, 0);
	
	// Pack the left Vbox into an HBox next to the preview widget on the right
	// The preview gets a Vbox of its own, to stop it from expanding vertically
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));
	hbx->pack_start(*leftVbx, true, true, 0);
	
	Gtk::VBox* previewBox = Gtk::manage(new Gtk::VBox(false, 0));
	previewBox->pack_start(*_modelPreview->getWidget(), false, false, 0);
	
	hbx->pack_start(*previewBox, false, false, 0);
	
	// Pack widgets into main Vbox above the buttons
	_vbox->pack_start(*hbx, true, true, 0);

	// Create the buttons below everything
	_vbox->pack_end(createButtons(), false, false, 0);
	_vbox->pack_end(createAdvancedButtons(), false, false, 0);

	add(*_vbox);
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
		GlobalRadiant().addEventListener(instancePtr);
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
	globalOutputStream() << "ModelSelector shutting down.\n";

	_modelPreview.reset();

	// Last step: reset the shared_ptr, triggers destruction of this instance
	InstancePtr().reset();
}

void ModelSelector::_postShow()
{
	// conditionally hide the options
	if (!_showOptions)
	{
		_advancedOptions->hide();
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

	updateSelected();

	_showOptions = showOptions;

	// show and enter recursive main loop. This will block until the dialog is hidden in some way.
	show(); 

	// Reset the preview model to release resources
	_modelPreview->clear();

	// Construct the model/skin combo and return it
	return ModelSelectorResult(
		_lastModel, 
		_lastSkin, 
		_clipCheckButton->get_active()
	);
}

// Static function to display the instance, and return the selected model to the 
// calling function
ModelSelectorResult ModelSelector::chooseModel(const std::string& curModel, bool showOptions, bool showSkins)
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
Gtk::Widget& ModelSelector::createTreeView() 
{
	// Create the treeview
	_treeView = Gtk::manage(new Gtk::TreeView(_treeStore));
	_treeView->set_headers_visible(false);

	// Single visible column, containing the directory/model name and the icon
	_treeView->append_column(*Gtk::manage(
		new gtkutil::IconTextColumn(_("Model Path"), _columns.filename, _columns.icon))); 
	
	// Use the TreeModel's full string search function
	_treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContainsmm));

	// Get the selection object and connect to its changed signal
	_selection = _treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &ModelSelector::callbackSelChanged));

	// Pack treeview into a scrolled window and frame, and return
	return *Gtk::manage(new gtkutil::ScrolledFramemm(*_treeView));
}

// Populate the tree view with models
void ModelSelector::populateModels() 
{
	// Clear the treestore first
	_treeStore->clear();
	_treeStoreWithSkins->clear();

	// Create a VFSTreePopulator for the treestore
	gtkutil::VFSTreePopulatormm pop(_treeStore);
	gtkutil::VFSTreePopulatormm popSkins(_treeStoreWithSkins);
	
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

// Create the buttons panel at bottom of dialog
Gtk::Widget& ModelSelector::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));
	
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ModelSelector::callbackOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ModelSelector::callbackCancel));
	
	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);
					   
	return *Gtk::manage(new gtkutil::RightAlignmentmm(*hbx));
}

// Create the advanced buttons panel
Gtk::Widget& ModelSelector::createAdvancedButtons()
{
	_advancedOptions = Gtk::manage(new Gtk::Expander(_("Advanced")));
	
	_clipCheckButton = Gtk::manage(new Gtk::CheckButton(_("Create MonsterClip Brush")));

	_advancedOptions->add(*_clipCheckButton);

	return *_advancedOptions;
}

// Create the info panel treeview
Gtk::Widget& ModelSelector::createInfoPanel()
{
	// Info table. Has key and value columns.
	Gtk::TreeView* infTreeView = Gtk::manage(new Gtk::TreeView(_infoStore));
	infTreeView->set_headers_visible(false);
	
	infTreeView->append_column(*Gtk::manage(
		new gtkutil::TextColumnmm(_("Attribute"), _infoStoreColumns.attribute)
	));

	infTreeView->append_column(*Gtk::manage(
		new gtkutil::TextColumnmm(_("Value"), _infoStoreColumns.value)
	));

	// Pack into scrolled frame and return
	return *Gtk::manage(new gtkutil::ScrolledFramemm(*infTreeView));
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

void ModelSelector::updateSelected()
{
	// Prepare to populate the info table
	_infoStore->clear();
	
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
	model::IModelPtr mdl = _modelPreview->getModel();
	if (!mdl) {
		return; // no valid model
	}
	
	// Update the text in the info table
	Gtk::TreeModel::Row row = *_infoStore->append();

	row[_infoStoreColumns.attribute] = std::string(_("Model name"));
	row[_infoStoreColumns.value] = mName;

	row = *_infoStore->append();
	row[_infoStoreColumns.attribute] = std::string(_("Skin name"));
	row[_infoStoreColumns.value] = skinName;
				   
	row = *_infoStore->append();
	row[_infoStoreColumns.attribute] = std::string(_("Total vertices"));
	row[_infoStoreColumns.value] = intToStr(mdl->getVertexCount());

	row = *_infoStore->append();
	row[_infoStoreColumns.attribute] = std::string(_("Total polys"));
	row[_infoStoreColumns.value] = intToStr(mdl->getPolyCount());

	row = *_infoStore->append();
	row[_infoStoreColumns.attribute] = std::string(_("Material surfaces"));
	row[_infoStoreColumns.value] = intToStr(mdl->getSurfaceCount());

	// Add the list of active materials
	const model::MaterialList& matList(mdl->getActiveMaterials());
	
	if (!matList.empty())
	{
		model::MaterialList::const_iterator i = matList.begin();

		// First line
		row = *_infoStore->append();
		row[_infoStoreColumns.attribute] = std::string(_("Active materials"));
		row[_infoStoreColumns.value] = *i;

		// Subsequent lines (if any)
		while (++i != matList.end())
		{
			row = *_infoStore->append();
			row[_infoStoreColumns.attribute] = std::string("");
			row[_infoStoreColumns.value] = *i;
		}
	}
}

void ModelSelector::callbackSelChanged()
{
	updateSelected();
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
