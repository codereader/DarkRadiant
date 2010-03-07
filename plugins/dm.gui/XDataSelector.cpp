#include "XDataSelector.h"

#include "imainframe.h"

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/dialog.h"

#include "XDataInserter.h"

namespace ui
{

namespace
{
	const std::string WINDOW_TITLE("Choose an XData Definition...");
	const gint WINDOW_WIDTH = 400;
	const gint WINDOW_HEIGHT = 500;
}

XDataSelector::XDataSelector(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
	_store(gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING,	G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN)),
	_editorDialog(editorDialog),
	_files(files),
	_result(RESULT_CANCELLED)
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), WINDOW_WIDTH, WINDOW_HEIGHT);

	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Add a vbox for the dialog elements
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(vbox), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), createButtons(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);

	fillTree();

	gtk_widget_set_sensitive(_okButton, FALSE);
}

std::string XDataSelector::run(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog)
{
	XDataSelector dialog(files, editorDialog);
	dialog.show();

	return (dialog._result == RESULT_OK) ? dialog._selection : "";
}

void XDataSelector::fillTree()
{
	// Start adding to tree.
	gtkutil::VFSTreePopulator populator(_store);

	for (XData::StringVectorMap::const_iterator it = _files.begin(); it != _files.end(); ++it)
	{
		populator.addPath(it->first);
	}

	XDataInserter inserter;
	populator.forEachNode(inserter);
}

GtkWidget* XDataSelector::createTreeView()
{
	// Create the treeview
	GtkTreeView* _treeView = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_store))
	);
	g_object_unref(_store);
	gtk_tree_view_set_headers_visible(_treeView, FALSE);

	// Add the selection and connect the signal
	GtkTreeSelection* select = gtk_tree_view_get_selection ( _treeView );
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(select, "changed", G_CALLBACK(onSelectionChanged), this);

	// Single visible column, containing the directory/model name and the icon
	GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn("Model Path", NAME_COLUMN, IMAGE_COLUMN);
	gtk_tree_view_append_column(_treeView, nameCol);				

	// Set the tree store's sort behaviour
	gtkutil::TreeModel::applyFoldersFirstSortFunc(
		GTK_TREE_MODEL(_store), NAME_COLUMN, IS_FOLDER_COLUMN
	);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(_treeView, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

	// Pack treeview into a scrolled window and frame, and return
	return gtkutil::ScrolledFrame(GTK_WIDGET(_treeView));
}

GtkWidget* XDataSelector::createButtons()
{
	_okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(_okButton), "clicked", G_CALLBACK(onOk), this);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);

	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(hbox), _okButton, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cancelButton, FALSE, FALSE, 0);

	return gtkutil::RightAlignment(hbox);
}

void XDataSelector::onCancel(GtkWidget* widget, XDataSelector* self)
{
	self->destroy();
}

void XDataSelector::onOk(GtkWidget* widget, XDataSelector* self)
{
	self->_result = RESULT_OK;

	// Everything done. Destroy the window!
	self->destroy();
}

void XDataSelector::onSelectionChanged(GtkTreeSelection* treeselection, XDataSelector* self)
{
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(treeselection, &model, NULL) ? true : false;

	if (anythingSelected && !gtkutil::TreeModel::getSelectedBoolean(treeselection, IS_FOLDER_COLUMN))
	{
		self->_selection = gtkutil::TreeModel::getSelectedString(treeselection, FULLNAME_COLUMN);
		self->_editorDialog->updateGuiView("", self->_selection);

		gtk_widget_set_sensitive(self->_okButton, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(self->_okButton, FALSE);
	}
}

} // namespace ui
