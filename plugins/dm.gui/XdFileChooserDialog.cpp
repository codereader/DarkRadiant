#include "XdFileChooserDialog.h"

#include "gtkutil/LeftAlignedLabel.h"
#include <gtk/gtk.h>
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "imainframe.h"

namespace ui
{

namespace
{
	const std::string WINDOW_TITLE("Choose a file...");
}

XdFileChooserDialog::XdFileChooserDialog(readable::XDataMap::iterator* fileIterator, readable::XDataMap* xdMap) : 
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
	_fileIterator(fileIterator),
	_xdMap(xdMap)
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Add a vbox for the dialog elements
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create topLabel and button
	GtkWidget* topLabel = gtkutil::LeftAlignedLabel("The requested definition has been found in multiple Files. Choose the file:");
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);

	// Create the list of files:
	GtkListStore* listStore = gtk_list_store_new(1, G_TYPE_STRING);
	_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));

	GtkTreeViewColumn* fileCol = gtkutil::TextColumn("Files", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeview), fileCol);

	// Append all xdMap-entries to the list.
	for (readable::XDataMap::const_iterator it = xdMap->begin(); it != xdMap->end(); it++)
	{
		GtkTreeIter iter;
		gtk_list_store_append(listStore, &iter);
		gtk_list_store_set(listStore, &iter, 0, it->first.c_str(), -1);
	}
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeview), false);
	gtk_widget_show(_treeview);

	g_object_unref(G_OBJECT(listStore));

	//Add everything to the vbox and to the window.
	gtk_box_pack_start(GTK_BOX(vbox), topLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _treeview, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), okButton, TRUE, TRUE, 0);	
	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);

	//Connect the OkButton-click command.
	g_signal_connect(
		G_OBJECT(okButton), "clicked", G_CALLBACK(onOk), this
		);
}

void XdFileChooserDialog::storeSelection()
{
	// Get the treeview selection and create the corresponding _fileIterator
	GtkTreeSelection *select;
	select = gtk_tree_view_get_selection ( GTK_TREE_VIEW (_treeview) );
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	std::string selStr = gtkutil::TreeModel::getSelectedString(select,0);
	if (selStr != "")
		*_fileIterator = _xdMap->find(selStr);

}

void XdFileChooserDialog::onOk(GtkWidget* widget, XdFileChooserDialog* self)
{
	self->storeSelection();

	self->destroy();
}

}
