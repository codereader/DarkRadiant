#include "GuiSelector.h"
#include "GuiInserter.h"

#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"

#include "imainframe.h"
#include "gui/GuiManager.h"
#include "gtkutil/dialog.h"

#include "ReadablePopulator.h"

namespace ui
{
	
namespace
{
	const std::string WINDOW_TITLE("Choose a Gui Definition...");

	const gint WINDOW_WIDTH = 400;
	const gint WINDOW_HEIGHT = 500;
}


GuiSelector::GuiSelector(bool twoSided, ReadableEditorDialog& editorDialog) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GTK_WINDOW(editorDialog.getWindow())),
	_editorDialog(editorDialog),
	_oneSidedStore(gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN)),
	_twoSidedStore(gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN)),
	_result(RESULT_CANCELLED)
{
	// Set the windowsize and default border width in accordance to the HIG
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), WINDOW_WIDTH, WINDOW_HEIGHT);

	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_container_add(GTK_CONTAINER(getWindow()), createInterface());

	// Set the current page and connect the switch-page signal afterwards.
	gtk_notebook_set_current_page(_notebook, twoSided);
	g_signal_connect(G_OBJECT(_notebook), "switch-page", G_CALLBACK(onPageSwitch), this);

	// We start with an empty selection, so de-sensitise the OK button
	gtk_widget_set_sensitive(_okButton, FALSE);
}

void GuiSelector::_preShow()
{
	// Call the base class
	BlockingTransientWindow::_preShow();

	// Populate the treestores
	fillTrees();
}

std::string GuiSelector::run(bool twoSided, ReadableEditorDialog& editorDialog)
{
	GuiSelector dialog(twoSided, editorDialog);

	try
	{
		dialog.show();
	}
	catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
	{
		return "";
	}

	return (dialog._result == RESULT_OK) ? "guis/" + dialog._name : "";
}

void GuiSelector::fillTrees()
{
	gtkutil::VFSTreePopulator popOne(_oneSidedStore);
	gtkutil::VFSTreePopulator popTwo(_twoSidedStore);

	ReadablePopulator walker(popOne, popTwo, GTK_WINDOW(_editorDialog.getWindow()));
	gui::GuiManager::Instance().foreachGui(walker);
	
	GuiInserter inserter;
	popOne.forEachNode(inserter);
	popTwo.forEachNode(inserter);
}

GtkWidget* GuiSelector::createInterface()
{
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the tabs
	_notebook = GTK_NOTEBOOK(gtk_notebook_new());

	// One-Sided Readables Tab
	GtkWidget* labelOne = gtk_label_new("One-Sided Readable Guis");
	gtk_widget_show_all(labelOne);
	gtk_notebook_append_page(
		_notebook,
		createOneSidedTreeView(),
		labelOne
	);

	// Two-Sided Readables Tab
	GtkWidget* labelTwo = gtk_label_new("Two-Sided Readable Guis");
	gtk_widget_show_all(labelTwo);
	gtk_notebook_append_page(
		_notebook,
		createTwoSidedTreeView(),
		labelTwo
	);

	// Packing
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(_notebook), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), createButtons(), FALSE, FALSE, 0);

	return vbox;
}

GtkWidget* GuiSelector::createButtons()
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

GtkWidget* GuiSelector::createOneSidedTreeView()
{
	// Create the treeview
	GtkTreeView* treeViewOne = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_oneSidedStore))
	);
	// Let the treestore be destroyed along with the treeview
	g_object_unref(_oneSidedStore);

	gtk_tree_view_set_headers_visible(treeViewOne, FALSE);

	// Add the selection and connect the signal
	GtkTreeSelection* select = gtk_tree_view_get_selection ( treeViewOne );
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(
		select, "changed", G_CALLBACK(onSelectionChanged), this
	);

	// Single visible column, containing the directory/model name and the icon
	GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn(
		"Gui Path", NAME_COLUMN, IMAGE_COLUMN
	);
	gtk_tree_view_append_column(treeViewOne, nameCol);

	// Set the tree store's sort behaviour
	gtkutil::TreeModel::applyFoldersFirstSortFunc(
		GTK_TREE_MODEL(_oneSidedStore), NAME_COLUMN, IS_FOLDER_COLUMN
	);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(treeViewOne, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

	GtkWidget* scrolledFrame = gtkutil::ScrolledFrame(GTK_WIDGET(treeViewOne));
	gtk_widget_show_all(scrolledFrame);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledFrame), 12);

	// Pack treeview into a scrolled window and frame, and return
	return scrolledFrame;
}

GtkWidget* GuiSelector::createTwoSidedTreeView()
{
	// Create the treeview
	GtkTreeView* treeViewTwo = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_twoSidedStore))
	);
	// Let the treestore be destroyed along with the treeview
	g_object_unref(_twoSidedStore);

	gtk_tree_view_set_headers_visible(treeViewTwo, FALSE);

	// Add selection and connect signal
	GtkTreeSelection* select = gtk_tree_view_get_selection ( treeViewTwo );
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(
		select, "changed", G_CALLBACK(onSelectionChanged), this
	);

	// Single visible column, containing the directory/model name and the icon
	GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn(
		"Gui Path", NAME_COLUMN, IMAGE_COLUMN
		);
	gtk_tree_view_append_column(treeViewTwo, nameCol);

	// Set the tree store's sort behaviour
	gtkutil::TreeModel::applyFoldersFirstSortFunc(
		GTK_TREE_MODEL(_twoSidedStore), NAME_COLUMN, IS_FOLDER_COLUMN
	);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(treeViewTwo, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

	GtkWidget* scrolledFrame = gtkutil::ScrolledFrame(GTK_WIDGET(treeViewTwo));
	gtk_widget_show_all(scrolledFrame);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledFrame), 12);

	// Pack treeview into a scrolled window and frame, and return
	return scrolledFrame;
}

void GuiSelector::onCancel(GtkWidget* widget, GuiSelector* self)
{
	self->destroy();
}

void GuiSelector::onOk(GtkWidget* widget, GuiSelector* self)
{
	self->_result = RESULT_OK;

	// Everything done. Destroy the window!
	self->destroy();
}

void GuiSelector::onPageSwitch(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, GuiSelector* self)
{
	if (page_num == 0)
		self->_editorDialog.useOneSidedEditing();
	else
		self->_editorDialog.useTwoSidedEditing();
}

void GuiSelector::onSelectionChanged(GtkTreeSelection* treeselection, GuiSelector* self)
{
	GtkTreeModel* model;
	bool anythingSelected = gtk_tree_selection_get_selected(treeselection, &model, NULL) ? true : false;

	if (anythingSelected && !gtkutil::TreeModel::getSelectedBoolean(treeselection, IS_FOLDER_COLUMN))
	{
		self->_name = gtkutil::TreeModel::getSelectedString(treeselection, FULLNAME_COLUMN);
		std::string guiPath = "guis/" + self->_name;

		self->_editorDialog.updateGuiView(GTK_WINDOW(self->getWindow()), guiPath.c_str());

		gtk_widget_set_sensitive(self->_okButton, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(self->_okButton, FALSE);
	}
}

} // namespace
