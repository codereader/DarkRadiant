#include "EClassTree.h"

#include <gtk/gtk.h>
#include "gtkutil/RightAlignment.h"

namespace ui {

EClassTree::EClassTree() :
	gtkutil::BlockingTransientWindow("Entity Class Tree", GlobalRadiant().getMainWindow())
{
	// Travese the entity defs and build the tree store
	_eclassStore = NULL;
	
	// Construct the window's widgets
	populateWindow();
	
	// Enter main loop
	show();
}

void EClassTree::populateWindow() {
	// Create the overall vbox
	_dialogVBox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), _dialogVBox);
	
	GtkWidget* panes = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(_dialogVBox), panes, FALSE, FALSE, 0);
	
	// Pack tree view
	gtk_paned_add1(GTK_PANED(panes), createEClassTreeView());
	
	// Pack spawnarg treeview
	//gtk_paned_add1(GTK_PANED(panes), GTK_WIDGET(_propertyTreeView));
	
	// Pack in dialog buttons
	gtk_box_pack_start(GTK_BOX(_dialogVBox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* EClassTree::createEClassTreeView() {
	_eclassTreeView = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_eclassStore))
	);
	return GTK_WIDGET(_eclassTreeView);
}

// Lower dialog buttons
GtkWidget* EClassTree::createButtons() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Close Button
	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(onClose), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), closeButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

// Static command target
void EClassTree::showWindow() {
	// Construct a new instance, this enters the main loop
	EClassTree _tree;
}

void EClassTree::onClose(GtkWidget* button, EClassTree* self) {
	self->destroy();
}

} // namespace ui
