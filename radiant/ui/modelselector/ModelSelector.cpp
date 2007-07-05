#include "ModelSelector.h"
#include "ModelFileFunctor.h"
#include "ModelDataInserter.h"

#include "mainframe.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "math/Vector3.h"
#include "ifilesystem.h"
#include "iregistry.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <ext/hash_map>
#include <sstream>
#include <GL/glew.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

// Constructor.

ModelSelector::ModelSelector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(gtk_tree_store_new(N_COLUMNS, 
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								GDK_TYPE_PIXBUF)),
  _infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING)),
  _lastModel(""),
  _lastSkin("")
{
	// Window properties
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), MODELSELECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_resizable(GTK_WINDOW(_widget), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);

	// Set the default size of the window
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	
	// Size the model preview widget
	float previewHeightFactor = boost::lexical_cast<float>(
		GlobalRegistry().get("user/ui/ModelSelector/previewSizeFactor"));
	gint glSize = gint(h * previewHeightFactor);
	_modelPreview.setSize(glSize);

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
	gtk_widget_set_size_request(leftVbx, w / 3, -1);
	
	// Pack the left Vbox into an HBox next to the preview widget on the right
	// The preview gets a Vbox of its own, to stop it from expanding vertically
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), leftVbx, TRUE, TRUE, 0);
	
	GtkWidget* previewBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(previewBox), _modelPreview, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbx), previewBox, FALSE, FALSE, 0);
	
	// Pack widgets into main Vbox above the buttons
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);

	// Create the buttons below everything
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createAdvancedButtons(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
	// Populate the tree of models
	populateModels();
}

// Show the dialog and enter recursive main loop

ModelSelectorResult ModelSelector::showAndBlock(bool showOptions) {

	// Show the dialog
	gtk_widget_show_all(_widget);

	// conditionally hide the options
	if (!showOptions)
		gtk_widget_hide(GTK_WIDGET(_advancedOptions));

	// Update the model preview widget, forcing an update of the selected model
	// since the preview model is deleted on dialog hide
	_modelPreview.initialisePreview();
	updateSelected();

	gtk_main(); // recursive main loop. This will block until the dialog is closed in some way.

	// Reset the preview model to release resources
	_modelPreview.setModel("");

	// Construct the model/skin combo and return it
	return ModelSelectorResult(_lastModel, _lastSkin, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_clipCheckButton)) );
}

// Static function to display the instance, and return the selected model to the 
// calling function
ModelSelectorResult ModelSelector::chooseModel(bool showOptions) {
	
	// Static instance pointer
	typedef boost::shared_ptr<ModelSelector> ModelSelectorPtr;
	static ModelSelectorPtr _selector;
	
	// Create the ModelSelector if it is not valid
	if (!_selector) {

		// Attempt to construct the static instance. This could throw an 
		// exception if the population of models is aborted by the user.
		try {
			_selector = ModelSelectorPtr(new ModelSelector());
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException e) {
			// Return a blank model and skin
			return ModelSelectorResult("", "", false);
		}
	}
	
	// Use the instance to select a model.
	return _selector->showAndBlock(showOptions);
}


// Helper function to create the TreeView
GtkWidget* ModelSelector::createTreeView() {

	// Create the treeview
	GtkWidget* treeView = 
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);

	// Single visible column, containing the directory/model name and the icon
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView),
								gtkutil::IconTextColumn("Value",
														NAME_COLUMN,
														IMAGE_COLUMN));				

	// Get the selection object and connect to its changed signal

	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelChanged), this);

	// Pack treeview into a scrolled window and frame, and return
	
	GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollWin), treeView);
	
	GtkWidget* fr = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(fr), scrollWin);
	
	return fr;
	
}

// Populate the tree view with models
void ModelSelector::populateModels() {

	// Create a VFSTreePopulator for the treestore
	gtkutil::VFSTreePopulator pop(_treeStore);
	
	// Use a ModelFileFunctor to add paths to the populator
	ModelFileFunctor functor(pop);
	GlobalFileSystem().forEachFile(MODELS_FOLDER, 
								   "*", 
								   makeCallback1(functor), 
								   0);
	
	// Fill in the column data
	ModelDataInserter inserter;
	pop.forEachNode(inserter);
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
	_modelPreview.setModel(mName);
	_modelPreview.setSkin(skinName);

	// Check that the model is actually valid by querying the IModelPtr 
	// returned from the preview widget.
	model::IModelPtr mdl = _modelPreview.getModel();
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
