#include "XDataSelector.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "imainframe.h"
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

	XDataSelector::XDataSelector(const XData::StringVectorMap& files) :
		gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
		_store(gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING,	G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN)),
		_files(files),
		_result("")
	{
		fillTree();

		gtk_window_set_default_size(GTK_WINDOW(getWindow()), WINDOW_WIDTH, WINDOW_HEIGHT);

		// Set the default border width in accordance to the HIG
		gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
		gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

		// Add a vbox for the dialog elements
		GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

		gtk_box_pack_start(GTK_BOX(vbox), createTreeView(), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), createButtons(), FALSE, FALSE, 0);

		gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
	}


	void XDataSelector::fillTree()
	{
		// Start adding to tree.
		gtkutil::VFSTreePopulator populator(_store);
		for (XData::StringVectorMap::const_iterator it = _files.begin(); it != _files.end(); it++)
		{
			populator.addPath(it->first);
		}
		XDataInserter inserter;
		populator.forEachNode(inserter);
	}

	GtkWidget* XDataSelector::createTreeView()
	{
		// Create the treeview
		_treeView = GTK_TREE_VIEW(
			gtk_tree_view_new_with_model(GTK_TREE_MODEL(_store))
			);
		gtk_tree_view_set_headers_visible(_treeView, FALSE);

		// Single visible column, containing the directory/model name and the icon
		GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn(
			"Model Path", NAME_COLUMN, IMAGE_COLUMN
			);
		gtk_tree_view_append_column(_treeView, nameCol);				

		// Set the tree stores to sort on this column
		gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(_store),
			NAME_COLUMN,
			GTK_SORT_ASCENDING
			);

		// Set the custom sort function
		gtk_tree_sortable_set_sort_func(
			GTK_TREE_SORTABLE(_store),
			NAME_COLUMN,		// sort column
			treeViewSortFunc,	// function
			this,				// userdata
			NULL				// no destroy notify
			);

		// Use the TreeModel's full string search function
		gtk_tree_view_set_search_equal_func(_treeView, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

		// Pack treeview into a scrolled window and frame, and return
		return gtkutil::ScrolledFrame(GTK_WIDGET(_treeView));
	}

	GtkWidget* XDataSelector::createButtons()
	{
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		g_signal_connect(
			G_OBJECT(okButton), "clicked", G_CALLBACK(onOk), this
			);
		GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
		g_signal_connect(
			G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this
			);

		GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

		gtk_box_pack_start(GTK_BOX(hbox), okButton, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), cancelButton, FALSE, FALSE, 0);

		// Align the hbox to the center
		GtkWidget* alignment = gtk_alignment_new(0.5,1,0,0);
		gtk_container_add(GTK_CONTAINER(alignment), hbox);

		return alignment;
	}

	std::string XDataSelector::run(const XData::StringVectorMap& files)
	{
		XDataSelector dialog(files);
		dialog.show();
		return dialog._result;
	}

	void XDataSelector::onCancel(GtkWidget* widget, XDataSelector* self)
	{
		self->destroy();
	}

	void XDataSelector::onOk(GtkWidget* widget, XDataSelector* self)
	{
		GtkTreeSelection *select;
		select = gtk_tree_view_get_selection ( GTK_TREE_VIEW (self->_treeView) );
		gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

		//Check if folder is selected.
		if (gtkutil::TreeModel::getSelectedBoolean(select, IS_FOLDER_COLUMN))
		{
			gtkutil::errorDialog("You have selected a folder. Please select an XData Definition!", GlobalMainFrame().getTopLevelWindow() );
			return;
		}

		// Get the treeview selection
		self->_result = gtkutil::TreeModel::getSelectedString(select,FULLNAME_COLUMN);

		// Everything done. Destroy the window!
		self->destroy();
	}

	gint XDataSelector::treeViewSortFunc(GtkTreeModel *model, 
		GtkTreeIter *a, 
		GtkTreeIter *b, 
		gpointer user_data)
	{
		// Check if A or B are folders
		bool aIsFolder = gtkutil::TreeModel::getBoolean(model, a, IS_FOLDER_COLUMN);
		bool bIsFolder = gtkutil::TreeModel::getBoolean(model, b, IS_FOLDER_COLUMN);

		if (aIsFolder) {
			// A is a folder, check if B is as well
			if (bIsFolder) {
				// A and B are both folders, compare names
				std::string aName = gtkutil::TreeModel::getString(model, a, NAME_COLUMN);
				std::string bName = gtkutil::TreeModel::getString(model, b, NAME_COLUMN);

				// greebo: We're not checking for equality here, XData names are unique
				return (aName < bName) ? -1 : 1;
			}
			else {
				// A is a folder, B is not, A sorts before
				return -1;
			}
		}
		else {
			// A is not a folder, check if B is one
			if (bIsFolder) {
				// A is not a folder, B is, so B sorts before A
				return 1;
			}
			else {
				// Neither A nor B are folders, compare names
				std::string aName = gtkutil::TreeModel::getString(model, a, NAME_COLUMN);
				std::string bName = gtkutil::TreeModel::getString(model, b, NAME_COLUMN);

				// greebo: We're not checking for equality here, XData names are unique
				return (aName < bName) ? -1 : 1;
			}
		}
	}
}