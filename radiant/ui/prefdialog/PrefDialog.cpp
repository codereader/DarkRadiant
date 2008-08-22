#include "PrefDialog.h"

#include <gtk/gtk.h>
#include "gtkutil/TextColumn.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/LeftAlignment.h"
#include "PrefPageWalkers.h"

#include "mainframe.h" // for UpdateAllWindows()

namespace ui {

PrefDialog::PrefDialog() :
	_dialog(NULL),
	_packed(false),
	_isModal(false)
{
	// Create a treestore with a name and a pointer
	_prefTree = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	
	// Create all the widgets
	populateWindow();
	
	// Create the root element with the Notebook and Connector references 
	_root = PrefPagePtr(new PrefPage("", "", _notebook, _registryConnector));
}

void PrefDialog::populateWindow() {
	// The overall dialog vbox
	_overallVBox = gtk_vbox_new(FALSE, 8);
	
	GtkWidget* hbox = gtk_hbox_new(FALSE, 8);
	
	_treeView = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_prefTree)));
	g_object_unref(G_OBJECT(_prefTree));
	
	_selection = gtk_tree_view_get_selection(_treeView);
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(onPrefPageSelect), this);
	
	gtk_tree_view_set_headers_visible(_treeView, FALSE);
	gtk_tree_view_append_column(_treeView, gtkutil::TextColumn("Category", 0)); 
	
	gtk_widget_set_size_request(GTK_WIDGET(_treeView), 170, -1);
	GtkWidget* scrolledFrame = gtkutil::ScrolledFrame(GTK_WIDGET(_treeView));
	gtk_box_pack_start(GTK_BOX(hbox), scrolledFrame, FALSE, FALSE, 0);
	
	_notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(_notebook), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), _notebook, TRUE, TRUE, 0);
	
	// Pack the notebook and the treeview into the overall dialog vbox
	gtk_box_pack_start(GTK_BOX(_overallVBox), hbox, TRUE, TRUE, 0);
	
	// Create the buttons
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 0);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onOK), this);
	
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	
	gtk_box_pack_start(GTK_BOX(_overallVBox), buttonHBox, FALSE, FALSE, 0);
}

void PrefDialog::updateTreeStore() {
	// Clear the tree before populating it
	gtk_tree_store_clear(_prefTree);
	
	// Instantiate a new populator class
	gtkutil::VFSTreePopulator vfsTreePopulator(_prefTree);
	
	PrefTreePopulator visitor(vfsTreePopulator, *this);
	
	// Visit each page with the PrefTreePopulator 
	// (which in turn is using the VFSTreePopulator helper)
	_root->foreachPage(visitor);
	
	// All the GtkTreeIters are available, we should add the data now
	// re-use the visitor, it provides both visit() methods
	vfsTreePopulator.forEachNode(visitor);
}

PrefPagePtr PrefDialog::createOrFindPage(const std::string& path) {
	// Pass the call to the root page
	return _root->createOrFindPage(path);
}

void PrefDialog::initDialog() {
	
	_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(_dialog), "DarkRadiant Preferences");
	gtk_window_set_modal(GTK_WINDOW(_dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER);
		
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(_dialog), 8);
	gtk_window_set_type_hint(GTK_WINDOW(_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	g_signal_connect(G_OBJECT(_dialog), "delete-event", G_CALLBACK(onDelete), this);
	
	gtk_container_add(GTK_CONTAINER(_dialog), _overallVBox);
}

void PrefDialog::onRadiantShutdown() {
	// De-register from the RadiantEventSystem
	GlobalRadiant().removeEventListener(InstancePtr());

	if (_dialog != NULL) {
		gtk_widget_hide(_dialog);
	}
}

void PrefDialog::toggleWindow(bool isModal) {
	// Pass the call to the utility methods that save/restore the window position
	if (_dialog != NULL && GTK_WIDGET_VISIBLE(_dialog)) {
		gtk_widget_hide_all(_dialog);
	}
	else {
		if (!_packed) {
			// Window container not created yet
			_packed = true;
			initDialog();
		}
		
		// Import the registry keys 
		_registryConnector.importValues();
		
		// Rebuild the tree and expand it
		updateTreeStore();
		gtk_tree_view_expand_all(_treeView);
		
		// Now show the dialog window again
		gtk_widget_show_all(_dialog);
		
		// Is there a specific page display request?
		if (!_requestedPage.empty()) {
			showPage(_requestedPage);
		}
		
		if (isModal) {
			_isModal = true;
			
			// Resize the window to fit the widgets exactly
			gtk_widget_set_size_request(_dialog, -1, -1);
			// Reposition the modal dialog, it has been reset by the size_request call
			gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER);
			// Enter the main loop, gtk_main_quit() is called by the buttons
			gtk_main();
		}
	}
}

void PrefDialog::toggle() {
	Instance().toggleWindow();
}

PrefDialogPtr& PrefDialog::InstancePtr() {
	static PrefDialogPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = PrefDialogPtr(new PrefDialog);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

PrefDialog& PrefDialog::Instance() {
	return *InstancePtr();
}

void PrefDialog::selectPage() {
	// Get the widget* pointer from the current selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(
		_selection, &model, &iter
	) ? true : false;
	
	if (anythingSelected) {
		// Retrieve the pointer from the current row and cast it to a GtkWidget*
		gpointer widgetPtr = gtkutil::TreeModel::getPointer(model, &iter, PREFPAGE_COL);
		GtkWidget* page = reinterpret_cast<GtkWidget*>(widgetPtr);
		
		int pagenum = gtk_notebook_page_num(GTK_NOTEBOOK(_notebook), page);
		if (gtk_notebook_get_current_page(GTK_NOTEBOOK(_notebook)) != pagenum) {
			gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook), pagenum);
		}
	}
}

void PrefDialog::showPage(const std::string& path) {
	PrefPagePtr page;
	
	PrefPageFinder finder(path, page);
	_root->foreachPage(finder);
	
	if (page != NULL) {
		GtkWidget* notebookPage = page->getWidget();
		int pagenum = gtk_notebook_page_num(GTK_NOTEBOOK(_notebook), notebookPage);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook), pagenum);
	}
}

void PrefDialog::save() {
	_registryConnector.exportValues();
	
	if (_isModal) {
		gtk_main_quit();
	}
	toggleWindow();
	_requestedPage = "";
	_isModal = false;
	UpdateAllWindows();
}

void PrefDialog::cancel() {
	if (_isModal) {
		gtk_main_quit();
	}
	toggleWindow();
	_requestedPage = "";
	_isModal = false;
}

void PrefDialog::showModal(const std::string& path) {
	if (!Instance().isVisible()) {
		Instance()._requestedPage = path;
		Instance().toggleWindow(true);
	}
}

void PrefDialog::showProjectSettings() {
	showModal("Game");
}

bool PrefDialog::isVisible() const {
	return (_dialog != NULL && GTK_WIDGET_VISIBLE(_dialog));
}

// Static GTK Callbacks
void PrefDialog::onOK(GtkWidget* button, PrefDialog* self) {
	self->save();
}

void PrefDialog::onCancel(GtkWidget* button, PrefDialog* self) {
	self->cancel();
} 

void PrefDialog::onPrefPageSelect(GtkTreeSelection* treeselection, PrefDialog* self) {
	self->selectPage();
}

gboolean PrefDialog::onDelete(GtkWidget* widget, GdkEvent* event, PrefDialog* self) {
	// Closing the dialog is equivalent to CANCEL
	self->cancel();

	// Don't propagate the delete event
	return true;
}

} // namespace ui
