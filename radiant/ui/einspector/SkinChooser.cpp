#include "SkinChooser.h"

#include "iradiant.h"
#include "modelskin.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/MultiMonitor.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* FOLDER_ICON = "folder16.png";
	const char* SKIN_ICON = "skin16.png";

	const std::string WINDOW_TITLE("Choose Skin");
	
	// Tree column enum
	enum {
		DISPLAYNAME_COL,
		FULLNAME_COL,
		ICON_COL,
		N_COLUMNS
	};
	
}

// Constructor
SkinChooser::SkinChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _lastSkin(""),
  _preview(new ModelPreview)
{
	// Set up window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), GlobalRadiant().getMainWindow());
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), WINDOW_TITLE.c_str());
	g_signal_connect(G_OBJECT(_widget), 
					 "delete-event", 
					 G_CALLBACK(_onCloseButton),
					 this);

	// Set the default size of the window
	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GTK_WINDOW(_widget));
	gint w = rect.width;

	// Main vbox
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);

	// HBox containing tree view and preview
	GtkWidget* hbx = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(hbx), createTreeView(w / 5), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), createPreview(w / 3), FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
}

SkinChooser& SkinChooser::Instance() {
	// Static instance pointer
	return *InstancePtr();
}

SkinChooserPtr& SkinChooser::InstancePtr() {
	static SkinChooserPtr _instancePtr;

	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = SkinChooserPtr(new SkinChooser);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}

	return _instancePtr;
}

// Create the TreeView
GtkWidget* SkinChooser::createTreeView(gint width) {
	
	// Create the treestore
	_treeStore = gtk_tree_store_new(N_COLUMNS, 
  									G_TYPE_STRING, 
  									G_TYPE_STRING,
  									GDK_TYPE_PIXBUF);
  									
	// Create the tree view
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	
	// Single column to display the skin name
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), 
								gtkutil::IconTextColumn("Skin", 
														DISPLAYNAME_COL, 
														ICON_COL));
	
	// Connect up selection changed callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	g_signal_connect(G_OBJECT(_selection), 
					 "changed", 
					 G_CALLBACK(_onSelChanged), 
					 this);
	
	// Pack treeview into a ScrolledFrame and return
	gtk_widget_set_size_request(_treeView, width, -1);
	return gtkutil::ScrolledFrame(_treeView);
}

// Create the model preview
GtkWidget* SkinChooser::createPreview(gint size) {
	_preview->setSize(size);
	return *_preview;
}

// Create the buttons panel
GtkWidget* SkinChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", 
					 G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);	
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
					   
	return gtkutil::RightAlignment(hbx);	
}

// Show the dialog and block for a selection
std::string SkinChooser::showAndBlock(const std::string& model,
									  const std::string& prev)
{
	// Set the model and previous skin, then populate the skins
	_model = model;
	_prevSkin = prev;
	populateSkins();

	// Display the model in the window title
	gtk_window_set_title(GTK_WINDOW(_widget), (WINDOW_TITLE + ": " + _model).c_str());
	
	// Show the dialog
	gtk_widget_show_all(_widget);
	_preview->initialisePreview();
	
	// Enter main loop and block
	gtk_main();
	
	// Hide the dialog and return the selection
	_preview->setModel(""); // release model
	gtk_widget_hide(_widget);
	return _lastSkin;
}

namespace {
	
/*
 * Visitor class to fill in column data for the skins tree.
 */
class SkinTreeVisitor
: public gtkutil::VFSTreePopulator::Visitor
{
public:

	// Required visit function
	void visit(GtkTreeStore* store, 
			   GtkTreeIter* it, 
			   const std::string& path,
			   bool isExplicit) 
	{
		// Get the display path, everything after rightmost slash
		std::string displayPath = path.substr(path.rfind("/") + 1);
		
		// Get the icon, either folder or skin
		GdkPixbuf* pixBuf = isExplicit
							? GlobalRadiant().getLocalPixbuf(SKIN_ICON)
							: GlobalRadiant().getLocalPixbuf(FOLDER_ICON);
		
		gtk_tree_store_set(store, it, 
						   DISPLAYNAME_COL, displayPath.c_str(),
						   FULLNAME_COL, path.c_str(),
						   ICON_COL, pixBuf,
						   -1);
	}
};

} // namespace

// Populate the list of skins
void SkinChooser::populateSkins() {
	
	// Clear the treestore
	gtk_tree_store_clear(_treeStore);
	
	// Add the "Matching skins" toplevel node
	GtkTreeIter matchingSkins;
	gtk_tree_store_append(_treeStore, &matchingSkins, NULL);
	gtk_tree_store_set(_treeStore, &matchingSkins, 
					   DISPLAYNAME_COL, "Matching skins", 
					   FULLNAME_COL, "",
					   ICON_COL, GlobalRadiant().getLocalPixbuf(FOLDER_ICON),
					   -1); 		

	// Get the skins for the associated model, and add them as matching skins
	const StringList& matchList = 
		GlobalModelSkinCache().getSkinsForModel(_model);
	for (StringList::const_iterator i = matchList.begin();
		 i != matchList.end();
		 ++i)
	{
		GtkTreeIter temp;
		gtk_tree_store_append(_treeStore, &temp, &matchingSkins);
		gtk_tree_store_set(_treeStore, &temp, 
						   DISPLAYNAME_COL, i->c_str(), 
						   FULLNAME_COL, i->c_str(),
						   ICON_COL, GlobalRadiant().getLocalPixbuf(SKIN_ICON),
						   -1); 		
	}
	
	// Add "All skins" toplevel node
	GtkTreeIter allSkins;
	gtk_tree_store_append(_treeStore, &allSkins, NULL);
	gtk_tree_store_set(_treeStore, &allSkins, 
					   DISPLAYNAME_COL, "All skins", 
					   FULLNAME_COL, "",
					   ICON_COL, GlobalRadiant().getLocalPixbuf(FOLDER_ICON),
					   -1); 		
	
	// Get the list of skins for the model
	const StringList& skins = GlobalModelSkinCache().getAllSkins();
	
	// Create a TreePopulator for the tree store and pass in each of the
	// skin names.
	gtkutil::VFSTreePopulator pop(_treeStore, &allSkins);

	for (StringList::const_iterator i = skins.begin();
		 i != skins.end();
		 ++i)
	{
		pop.addPath(*i);
	}
	
	// Visit the tree populator in order to fill in the column data
	SkinTreeVisitor visitor;
	pop.forEachNode(visitor);
}

// Static method to display singleton instance and choose a skin
std::string SkinChooser::chooseSkin(const std::string& model,
									const std::string& prev) 
{
	// Show and block the instance, returning the selected skin
	return Instance().showAndBlock(model, prev);	
}

void SkinChooser::onRadiantShutdown() {
	globalOutputStream() << "SkinChooser shutting down.\n";

	_preview = ModelPreviewPtr();
}

/* GTK CALLBACKS */

void SkinChooser::_onOK(GtkWidget* widget, SkinChooser* self) {

	// Get the selected skin
	self->_lastSkin = gtkutil::TreeModel::getSelectedString(self->_selection, 
															FULLNAME_COL);
	// Exit main loop
	gtk_main_quit();
}

void SkinChooser::_onCancel(GtkWidget* widget, SkinChooser* self) {
	// Clear the last skin and quit the main loop
	self->_lastSkin = self->_prevSkin;
	gtk_main_quit();	
}

void SkinChooser::_onSelChanged(GtkWidget*, SkinChooser* self) {

	// Get the selected skin
	std::string skin = gtkutil::TreeModel::getSelectedString(self->_selection, 
															 FULLNAME_COL);

	// Set the model preview to show the model with the selected skin
	self->_preview->setModel(self->_model);
	self->_preview->setSkin(skin);
}

gboolean SkinChooser::_onCloseButton(GtkWidget *widget, GdkEvent* ev, SkinChooser* self)
{
	// Clear the last skin and quit the main loop
	self->_lastSkin = self->_prevSkin;
	gtk_main_quit();
	return true;
}

} // namespace ui
