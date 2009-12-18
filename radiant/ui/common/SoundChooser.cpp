#include "SoundChooser.h"

#include "iuimanager.h"
#include "isound.h"
#include "iradiant.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/VFSTreePopulator.h"

#include <gtk/gtk.h>

namespace ui
{

namespace {
	
	// Treestore enum
	enum {
		DISPLAYNAME_COLUMN,
		SHADERNAME_COLUMN,
		IS_FOLDER_COLUMN,
		ICON_COLUMN,
		N_COLUMNS
	};	

	const char* const SHADER_ICON = "icon_sound.png";
	const char* const FOLDER_ICON = "folder16.png";	
}

// Constructor
SoundChooser::SoundChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF))
{
	// Set up the window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), GlobalRadiant().getMainWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose sound");
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(_widget), GDK_WINDOW_TYPE_HINT_DIALOG);
    
	// Set the default size of the window
	GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GTK_WINDOW(_widget));
	gtk_window_set_default_size(GTK_WINDOW(_widget), rect.width / 2, rect.height / 2);

    // Delete event
    g_signal_connect(
    	G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this
    );
    
    // Main vbox
    GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
    gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbx), _preview, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
    
    gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
    gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

namespace {

/**
 * Visitor class to enumerate sound shaders and add them to the tree store.
 */
class SoundShaderPopulator :
	public SoundShaderVisitor,
	public gtkutil::VFSTreePopulator,
	public gtkutil::VFSTreePopulator::Visitor
{
public:
	// Constructor
	SoundShaderPopulator(GtkTreeStore* treeStore) :
		gtkutil::VFSTreePopulator(treeStore)
	{}
	
	// Functor operator needed for the forEachShader() call
	void visit(const ISoundShaderPtr& shader) 
	{
		// Construct a "path" into the sound shader tree,
		// using the mod name as first folder level
		addPath(shader->getModName() + "/" + shader->getName());
	}

	// Required visit function
	void visit(GtkTreeStore* store, GtkTreeIter* iter, 
			   const std::string& path, bool isExplicit)
	{
		// Get the display name by stripping off everything before the last
		// slash
		std::string displayName = path.substr(path.rfind("/") + 1);

		// Pixbuf depends on model type
		GdkPixbuf* pixBuf = isExplicit 
							? GlobalUIManager().getLocalPixbuf(SHADER_ICON)
							: GlobalUIManager().getLocalPixbuf(FOLDER_ICON);

		// Fill in the column values
		gtk_tree_store_set(store, iter,
						   DISPLAYNAME_COLUMN, displayName.c_str(),
						   SHADERNAME_COLUMN, displayName.c_str(),
						   IS_FOLDER_COLUMN, isExplicit ? FALSE : TRUE,
						   ICON_COLUMN, pixBuf,
						   -1);
	}
};


} // namespace

// Create the tree view
GtkWidget* SoundChooser::createTreeView() {
	
	// Tree view with single text icon column
	GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv),
		gtkutil::IconTextColumn("Shader", DISPLAYNAME_COLUMN, ICON_COLUMN, false)
	);

	_treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	g_signal_connect(G_OBJECT(_treeSelection), "changed", 
					 G_CALLBACK(_onSelectionChange), this);

	// Populate the tree store with sound shaders, using a VFS tree populator
	SoundShaderPopulator pop(_treeStore);

	// Visit all sound shaders and collect them for later insertion
	GlobalSoundManager().forEachShader(pop);

	// Let the populator iterate over all collected elements 
	// and insert them in the treestore
	pop.forEachNode(pop);

	return gtkutil::ScrolledFrame(tv);	
}

// Create buttons panel
GtkWidget* SoundChooser::createButtons() {
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

// Show and block
std::string SoundChooser::chooseSound() {
	gtk_widget_show_all(_widget);
	gtk_main();
	return _selectedShader;	
}

/* GTK CALLBACKS */

// Delete dialog
gboolean SoundChooser::_onDelete(GtkWidget* w, GdkEvent* e, SoundChooser* self) {
	self->_selectedShader = "";
	gtk_main_quit();
	return false; // propagate event
}

// OK button
void SoundChooser::_onOK(GtkWidget* w, SoundChooser* self) {
	
	// Get the selection if valid, otherwise ""
	self->_selectedShader = 
		gtkutil::TreeModel::getSelectedString(self->_treeSelection,
											  SHADERNAME_COLUMN);
	
	gtk_widget_destroy(self->_widget);
	gtk_main_quit();	
}

// Cancel button
void SoundChooser::_onCancel(GtkWidget* w, SoundChooser* self) {
	self->_selectedShader = "";
	gtk_widget_destroy(self->_widget);
	gtk_main_quit();	
}

// Tree Selection Change
void SoundChooser::_onSelectionChange(GtkTreeSelection* selection, SoundChooser* self)
{
	bool isFolder = gtkutil::TreeModel::getSelectedBoolean(self->_treeSelection, IS_FOLDER_COLUMN);

	if (isFolder) 
	{
		self->_preview.setSoundShader("");
		return;
	}

	std::string selectedShader = gtkutil::TreeModel::getSelectedString(
		self->_treeSelection,
		SHADERNAME_COLUMN
	);
	
	// Notify the preview widget about the change 
	self->_preview.setSoundShader(selectedShader);
}

}
