#include "ModelSelector.h"
#include "ModelFileFunctor.h"
#include "ModelDataInserter.h"

#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "math/Vector3.h"
#include "ifilesystem.h"
#include "itextstream.h"
#include "iregistry.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <GL/glew.h>

#include "generic/callback.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

// Constructor.

ModelSelector::ModelSelector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _modelPreview(new ModelPreview),
  _treeStore(gtk_tree_store_new(N_COLUMNS, 
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								GDK_TYPE_PIXBUF)),
  _treeStoreWithSkins(gtk_tree_store_new(N_COLUMNS, 
  										G_TYPE_STRING,
  										G_TYPE_STRING,
  										G_TYPE_STRING,
  										GDK_TYPE_PIXBUF)),
  _infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING)),
  _lastModel(""),
  _lastSkin(""),
  _populated(false)
{
	// Window properties
	gtk_window_set_transient_for(GTK_WINDOW(_widget), GlobalRadiant().getMainWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), MODELSELECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);

	_position.connect(GTK_WINDOW(_widget));

	// Size the model preview widget
	float previewHeightFactor = GlobalRegistry().getFloat(
		"user/ui/ModelSelector/previewSizeFactor"
	);

	// Set the default size of the window
	_position.readPosition();
	_position.fitToScreen(0.8f, previewHeightFactor);
	_position.applyPosition();

	_modelPreview->setSize(_position.getSize()[1]);

	// Re-center the window
	gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Signals
	g_signal_connect(G_OBJECT(_widget), 
					 "delete_event", 
					 G_CALLBACK(callbackHide), 
					 this);
	
	// Main window contains a VBox. On the top goes the widgets, the bottom
	// contains the button panel
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	
	// Pack the tree view into a VBox above the info panel
	GtkWidget* leftVbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(leftVbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(leftVbx), createInfoPanel(), TRUE, TRUE, 0);
	
	// Pack the left Vbox into an HBox next to the preview widget on the right
	// The preview gets a Vbox of its own, to stop it from expanding vertically
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), leftVbx, TRUE, TRUE, 0);
	
	GtkWidget* previewBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(previewBox), *_modelPreview, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbx), previewBox, FALSE, FALSE, 0);
	
	// Pack widgets into main Vbox above the buttons
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);

	// Create the buttons below everything
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createAdvancedButtons(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

ModelSelector& ModelSelector::Instance() {
	// Static instance pointer
	return *InstancePtr();
}

ModelSelectorPtr& ModelSelector::InstancePtr() {
	static ModelSelectorPtr _instancePtr;

	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = ModelSelectorPtr(new ModelSelector);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}

	return _instancePtr;
}

void ModelSelector::onRadiantShutdown() {
	globalOutputStream() << "ModelSelector shutting down.\n";

	_modelPreview = ModelPreviewPtr();
}

// Show the dialog and enter recursive main loop
ModelSelectorResult ModelSelector::showAndBlock(const std::string& curModel,
                                                bool showOptions,
                                                bool showSkins) 
{
	if (!_populated) {
		// Attempt to construct the static instance. This could throw an 
		// exception if the population of models is aborted by the user.
		try {
			// Populate the tree of models
			populateModels();
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException e) {
			// Return a blank model and skin
			return ModelSelectorResult("", "", false);
		}
	}
	
	// Show the dialog
	gtk_widget_show_all(_widget);

	// conditionally hide the options
	if (!showOptions) {
		gtk_widget_hide(GTK_WIDGET(_advancedOptions));
	}

	// Choose the model based on the "showSkins" setting
	gtk_tree_view_set_model(
		GTK_TREE_VIEW(_treeView), 
		(showSkins) ? GTK_TREE_MODEL(_treeStoreWithSkins) : GTK_TREE_MODEL(_treeStore)
	);

	// If an empty string was passed for the current model, use the last selected one
	std::string previouslySelected = (!curModel.empty()) ? curModel : _lastModel;

	if (!previouslySelected.empty()) {
		// Lookup the model path in the treemodel
		gtkutil::TreeModel::SelectionFinder finder(previouslySelected, FULLNAME_COLUMN);
		
		GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(_treeView));
		gtk_tree_model_foreach(model, gtkutil::TreeModel::SelectionFinder::forEach, &finder);
		
		// Get the found TreePath (may be NULL)
		GtkTreePath* path = finder.getPath();
		if (path != NULL) {
			// Expand the treeview to display the target row
			gtk_tree_view_expand_to_path(_treeView, path);
			// Highlight the target row
			gtk_tree_view_set_cursor(_treeView, path, NULL, false);
			// Make the selected row visible 
			gtk_tree_view_scroll_to_cell(_treeView, path, NULL, true, 0.3f, 0.0f);
		}
	}

	// Update the model preview widget, forcing an update of the selected model
	// since the preview model is deleted on dialog hide
	_modelPreview->initialisePreview();
	updateSelected();

	gtk_main(); // recursive main loop. This will block until the dialog is closed in some way.

	// Reset the preview model to release resources
	_modelPreview->clear();

	// Construct the model/skin combo and return it
	return ModelSelectorResult(
		_lastModel, 
		_lastSkin, 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_clipCheckButton)) ? true : false 
	);
}

// Static function to display the instance, and return the selected model to the 
// calling function
ModelSelectorResult ModelSelector::chooseModel(const std::string& curModel, bool showOptions, bool showSkins) {
	// Use the instance to select a model.
	return Instance().showAndBlock(curModel, showOptions, showSkins);
}

void ModelSelector::refresh() {
	// Clear the flag, this triggers a new population next time the dialog is shown
	Instance()._populated = false;
}

// Helper function to create the TreeView
GtkWidget* ModelSelector::createTreeView() 
{
	// Create the treeview
	_treeView = GTK_TREE_VIEW(
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore))
    );
	gtk_tree_view_set_headers_visible(_treeView, FALSE);

	// Single visible column, containing the directory/model name and the icon
	GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn(
		"Model Path", NAME_COLUMN, IMAGE_COLUMN
	);
	gtk_tree_view_append_column(_treeView, nameCol);				

    // Set the tree stores to sort on this column
    gtk_tree_sortable_set_sort_column_id(
        GTK_TREE_SORTABLE(_treeStore),
        NAME_COLUMN,
        GTK_SORT_ASCENDING
    );
    gtk_tree_sortable_set_sort_column_id(
        GTK_TREE_SORTABLE(_treeStoreWithSkins),
        NAME_COLUMN,
        GTK_SORT_ASCENDING
    );

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(_treeView, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

	// Get the selection object and connect to its changed signal
	_selection = gtk_tree_view_get_selection(_treeView);
	g_signal_connect(
        G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelChanged), this
    );

	// Pack treeview into a scrolled window and frame, and return
	return gtkutil::ScrolledFrame(GTK_WIDGET(_treeView));
}

// Populate the tree view with models
void ModelSelector::populateModels() 
{
	// Clear the treestore first
	gtk_tree_store_clear(_treeStore);
	gtk_tree_store_clear(_treeStoreWithSkins);

	// Create a VFSTreePopulator for the treestore
	gtkutil::VFSTreePopulator pop(_treeStore);
	gtkutil::VFSTreePopulator popSkins(_treeStoreWithSkins);
	
	// Use a ModelFileFunctor to add paths to the populator
	ModelFileFunctor functor(pop, popSkins);
	GlobalFileSystem().forEachFile(MODELS_FOLDER, 
								   "*", 
								   makeCallback1(functor), 
								   0);
	
	// Fill in the column data (TRUE = including skins)
	ModelDataInserter inserterSkins(true);
	popSkins.forEachNode(inserterSkins);

	// Insert data into second model (FALSE = without skins)
	ModelDataInserter inserter(false);
	pop.forEachNode(inserter);
	
	// Set the flag, we're done	
	_populated = true;
}

// Create the buttons panel at bottom of dialog
GtkWidget* ModelSelector::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", 
					 G_CALLBACK(callbackCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(hbx);
}

// Create the advanced buttons panel
GtkWidget* ModelSelector::createAdvancedButtons() {
	_advancedOptions = GTK_EXPANDER(gtk_expander_new("advanced"));
	_clipCheckButton = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("create MonsterClip brush"));
	gtk_container_add(GTK_CONTAINER(_advancedOptions), GTK_WIDGET(_clipCheckButton));
	return GTK_WIDGET(_advancedOptions);
}

// Create the info panel treeview
GtkWidget* ModelSelector::createInfoPanel() {

	// Info table. Has key and value columns.
	GtkWidget* infTreeView = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_infoStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(infTreeView), FALSE);
	
	GtkCellRenderer* rend;
	GtkTreeViewColumn* col;
	
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Attribute",
												   rend,
												   "text", 0,
												   NULL);
	g_object_set(G_OBJECT(rend), "weight", 700, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(infTreeView), col);
	
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Value",
												   rend,
												   "text", 1,
												   NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(infTreeView), col);
	
	// Pack into scrolled frame and return
	return gtkutil::ScrolledFrame(infTreeView);
}

// Get the value from the selected column
std::string ModelSelector::getSelectedValue(gint colNum) {
	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;

	if (gtk_tree_selection_get_selected(_selection, &model, &iter)) {
		return gtkutil::TreeModel::getString(model, &iter, colNum);	
	}
	else {
		// Nothing selected, return empty string
		return "";
	}
}

// Update the info table and model preview based on the current selection

void ModelSelector::updateSelected() {

	// Prepare to populate the info table
	gtk_list_store_clear(_infoStore);
	GtkTreeIter iter;
	
	// Get the model name, if this is blank we are looking at a directory,
	// so leave the table empty
	std::string mName = getSelectedValue(FULLNAME_COLUMN);
	if (mName.empty())
		return;
	
	// Get the skin if set
	std::string skinName = getSelectedValue(SKIN_COLUMN);

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
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "Model name",
					   1, mName.c_str(),
					   -1);
					   
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "Skin name",
					   1, skinName.c_str(),
					   -1);

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter,
					   0, "Total vertices",
					   1, boost::lexical_cast<std::string>(mdl->getVertexCount()).c_str(),
					   -1);

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter,
					   0, "Total polys",
					   1, boost::lexical_cast<std::string>(mdl->getPolyCount()).c_str(),
					   -1);

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter,
					   0, "Material surfaces",
					   1, boost::lexical_cast<std::string>(mdl->getSurfaceCount()).c_str(),
					   -1);

	// Add the list of active materials
	const std::vector<std::string>& matList(mdl->getActiveMaterials());
	
	if (!matList.empty()) {
		std::vector<std::string>::const_iterator i = matList.begin();
		// First line		
		gtk_list_store_append(_infoStore, &iter);
		gtk_list_store_set(_infoStore, &iter,
						   0, "Active materials",
						   1, i->c_str(),
						   -1);
		// Subsequent lines (if any)
		while (++i != matList.end()) {
			gtk_list_store_append(_infoStore, &iter);
			gtk_list_store_set(_infoStore, &iter,
							   0, "",
							   1, i->c_str(),
							   -1);
		}
	}

}

/* GTK CALLBACKS */

void ModelSelector::callbackHide(GtkWidget* widget, GdkEvent* ev, ModelSelector* self) {
	self->_lastModel = "";
	self->_lastSkin = "";
	gtk_main_quit(); // exit recursive main loop
	gtk_widget_hide(self->_widget);
}

void ModelSelector::callbackSelChanged(GtkWidget* widget, ModelSelector* self) {
	self->updateSelected();
}

void ModelSelector::callbackOK(GtkWidget* widget, ModelSelector* self) {
	// Remember the selected model then exit from the recursive main loop
	self->_lastModel = self->getSelectedValue(FULLNAME_COLUMN);
	self->_lastSkin = self->getSelectedValue(SKIN_COLUMN);
	gtk_main_quit();
	gtk_widget_hide(self->_widget);
}

void ModelSelector::callbackCancel(GtkWidget* widget, ModelSelector* self) {
	self->_lastModel = "";
	self->_lastSkin = "";
	gtk_main_quit();
	gtk_widget_hide(self->_widget);
}


} // namespace ui
