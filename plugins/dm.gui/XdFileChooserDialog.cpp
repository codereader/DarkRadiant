#include "XdFileChooserDialog.h"

#include "gtkutil/LeftAlignedLabel.h"
#include <gtk/gtk.h>
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "imainframe.h"
#include "gtkutil/ScrolledFrame.h"

namespace ui
{

	namespace
	{
		const std::string WINDOW_TITLE("Choose a file...");
	}

	XdFileChooserDialog::Result XdFileChooserDialog::import(const std::string& defName, XData::XDataPtr& newXData, std::string& filename, XData::XDataLoaderPtr& loader)
	{
		// Import the file:
		XData::XDataMap xdMap;
		if ( loader->importDef(defName,xdMap) )
		{
			if (xdMap.size() > 1)
			{
				// The requested definition has been defined in multiple files. Use the XdFileChooserDialog to pick a file.
				// Optimally, the preview renderer would already show the selected definition.
				XdFileChooserDialog fcDialog(xdMap);
				fcDialog.show();
				if (fcDialog._result == RESULT_CANCEL)
					//User clicked cancel. The window will be destroyed in _postShow()...
					return RESULT_CANCEL;

				XData::XDataMap::iterator ChosenIt = xdMap.find(fcDialog._chosenFile);
				filename = ChosenIt->first;
				newXData = ChosenIt->second;
			}
			else
			{
				filename = xdMap.begin()->first;
				newXData = xdMap.begin()->second;
			}
			return RESULT_OK;
		}
		//Import failed.
		return RESULT_IMPORT_FAILED;
	}

	XdFileChooserDialog::XdFileChooserDialog(const XData::XDataMap& xdMap) : 
		gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
		_result(RESULT_CANCEL)
	{
		// Set the default border width in accordance to the HIG
		gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
		gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

		// Add a vbox for the dialog elements
		GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

		// Create topLabel
		GtkWidget* topLabel = gtkutil::LeftAlignedLabel("The requested definition has been found in multiple Files. Choose the file:");

		// Create the list of files:
		GtkListStore* listStore = gtk_list_store_new(1, G_TYPE_STRING);
		_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));

		GtkTreeViewColumn* fileCol = gtkutil::TextColumn("Files", 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(_treeview), fileCol);

		// Append all xdMap-entries to the list.
		for (XData::XDataMap::const_iterator it = xdMap.begin(); it != xdMap.end(); it++)
		{
			GtkTreeIter iter;
			gtk_list_store_append(listStore, &iter);
			gtk_list_store_set(listStore, &iter, 0, it->first.c_str(), -1);
		}
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeview), false);
		gtk_widget_show(_treeview);

		g_object_unref(G_OBJECT(listStore));

		// Create buttons and add them to an hbox:
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
		GtkWidget* hbox = gtk_hbox_new(TRUE, 6);
		gtk_box_pack_start(GTK_BOX(hbox), okButton, TRUE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), cancelButton, TRUE, FALSE, 0);

		//Add everything to the vbox and to the window.
		gtk_box_pack_start(GTK_BOX(vbox), topLabel, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), gtkutil::ScrolledFrame(GTK_WIDGET(_treeview)), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);	
		gtk_container_add(GTK_CONTAINER(getWindow()), vbox);

		//Connect the Signals.
		g_signal_connect(
			G_OBJECT(okButton), "clicked", G_CALLBACK(onOk), this
			);
		g_signal_connect(
			G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this
			);
	}

	void XdFileChooserDialog::storeSelection()
	{
		// Get the treeview selection and create the corresponding _fileIterator
		GtkTreeSelection *select;
		select = gtk_tree_view_get_selection ( GTK_TREE_VIEW (_treeview) );
		gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

		_chosenFile = gtkutil::TreeModel::getSelectedString(select,0);
	}

	void XdFileChooserDialog::onOk(GtkWidget* widget, XdFileChooserDialog* self)
	{
		self->storeSelection();
		self->_result = RESULT_OK;

		self->destroy();
	}

	void XdFileChooserDialog::onCancel(GtkWidget* widget, XdFileChooserDialog* self)
	{
		self->destroy();
	}

}
