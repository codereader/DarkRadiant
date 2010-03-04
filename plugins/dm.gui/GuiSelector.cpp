#include "GuiSelector.h"
#include "GuiInserter.h"

#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/ScrolledFrame.h"

#include "imainframe.h"
#include "gui/GuiManager.h"
#include "gtkutil/dialog.h"

namespace ui
{

namespace
{
	const std::string WINDOW_TITLE("Choose a Gui Definition...");

	const gint WINDOW_WIDTH = 400;
	const gint WINDOW_HEIGHT = 500;
}


GuiSelector::GuiSelector(bool twoSided, ReadableEditorDialog* editorDialog) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
	_editorDialog(editorDialog),
	_name("")
{
	static bool load = fillTrees();

	gtk_window_set_default_size(GTK_WINDOW(getWindow()), WINDOW_WIDTH, WINDOW_HEIGHT);

	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_container_add(GTK_CONTAINER(getWindow()), createInterface());

	// Set the current page and connect the switch-page signal afterwards.
	gtk_notebook_set_current_page(_notebook, twoSided);
	g_signal_connect(
		G_OBJECT(_notebook), "switch-page", G_CALLBACK(onPageSwitch), this
		);
}

std::string GuiSelector::run(bool twoSided, ReadableEditorDialog* editorDialog)
{
	GuiSelector dialog(twoSided, editorDialog);
	dialog.show();

	if (dialog._name.empty())
		return "";
	return "guis/" + dialog._name;
}

GtkTreeStore* GuiSelector::getOneSidedStore()
{
	static GtkTreeStore* _oneSidedStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN);
	return _oneSidedStore;
}

GtkTreeStore* GuiSelector::getTwoSidedStore()
{
	static GtkTreeStore* _twoSidedStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN);
	return _twoSidedStore;
}

bool GuiSelector::fillTrees()
{
	gtkutil::VFSTreePopulator popOne(getOneSidedStore());
	gtkutil::VFSTreePopulator popTwo(getTwoSidedStore());

	gui::GuiManager::GuiMap m = gui::GuiManager::Instance().getGuiDefinitions();
	
	for (gui::GuiManager::GuiMap::iterator it = m.begin(); it != m.end(); it++)
	{
		//identify readables and differ between onesided and twosided ones. Add the gui to the according tree.
		gui::GuiManager::GuiAppearance appearance = gui::GuiManager::Instance().checkGuiAppearance(it->second);
		if ( appearance == gui::GuiManager::ONE_SIDED_READABLE )
			popOne.addPath(it->first.substr(it->first.find("/")+1));	//ommit the guis-folder
		else if (appearance == gui::GuiManager::TWO_SIDED_READABLE)
			popTwo.addPath(it->first.substr(it->first.find("/")+1));	//ommit the guis-folder
	}

	GuiInserter inserter;
	popOne.forEachNode(inserter);
	popTwo.forEachNode(inserter);

	return true;
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

GtkWidget* GuiSelector::createOneSidedTreeView()
{
	// Create the treeview
	GtkTreeView* treeViewOne = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(getOneSidedStore()))
		);
	gtk_tree_view_set_headers_visible(treeViewOne, FALSE);

	// Add the selection and connect the signal
	GtkTreeSelection* select = gtk_tree_view_get_selection ( treeViewOne );
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(
		select, "changed", G_CALLBACK(onSelectionChanged), this
		);

	// Single visible column, containing the directory/model name and the icon
	GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn(
		"Model Path", NAME_COLUMN, IMAGE_COLUMN
		);
	gtk_tree_view_append_column(treeViewOne, nameCol);				

	// Set the tree stores to sort on this column
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(getOneSidedStore()),
		NAME_COLUMN,
		GTK_SORT_ASCENDING
		);

	// Set the custom sort function
	gtk_tree_sortable_set_sort_func(
		GTK_TREE_SORTABLE(getOneSidedStore()),
		NAME_COLUMN,		// sort column
		treeViewSortFunc,	// function
		this,				// userdata
		NULL				// no destroy notify
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
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(getTwoSidedStore()))
		);
	gtk_tree_view_set_headers_visible(treeViewTwo, FALSE);

	// Add selection and connect signal
	GtkTreeSelection* select = gtk_tree_view_get_selection ( treeViewTwo );
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(
		select, "changed", G_CALLBACK(onSelectionChanged), this
		);

	// Single visible column, containing the directory/model name and the icon
	GtkTreeViewColumn* nameCol = gtkutil::IconTextColumn(
		"Model Path", NAME_COLUMN, IMAGE_COLUMN
		);
	gtk_tree_view_append_column(treeViewTwo, nameCol);				

	// Set the tree stores to sort on this column
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(getTwoSidedStore()),
		NAME_COLUMN,
		GTK_SORT_ASCENDING
		);

	// Set the custom sort function
	gtk_tree_sortable_set_sort_func(
		GTK_TREE_SORTABLE(getTwoSidedStore()),
		NAME_COLUMN,		// sort column
		treeViewSortFunc,	// function
		this,				// userdata
		NULL				// no destroy notify
		);

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(treeViewTwo, gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

	GtkWidget* scrolledFrame = gtkutil::ScrolledFrame(GTK_WIDGET(treeViewTwo));
	gtk_widget_show_all(scrolledFrame);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledFrame), 12);

	// Pack treeview into a scrolled window and frame, and return
	return scrolledFrame;
}

gint GuiSelector::treeViewSortFunc(GtkTreeModel *model, 
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

void GuiSelector::onCancel(GtkWidget* widget, GuiSelector* self)
{
	self->_name.clear();
	self->destroy();
}

void GuiSelector::onOk(GtkWidget* widget, GuiSelector* self)
{	
	// Check if a gui has been chosen:
	if (self->_name.empty())
	{
		gtkutil::errorDialog("You have selected a folder. Please select a Gui Definition!", GlobalMainFrame().getTopLevelWindow() );
		return;
	}

	// Everything done. Destroy the window!
	self->destroy();
}

void GuiSelector::onPageSwitch(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, GuiSelector* self)
{
	if (page_num == 0)
		self->_editorDialog->useOneSidedEditing();
	else
		self->_editorDialog->useTwoSidedEditing();
}

void GuiSelector::onSelectionChanged(GtkTreeSelection *treeselection, GuiSelector* self)
{
	if (!gtkutil::TreeModel::getSelectedBoolean(treeselection, IS_FOLDER_COLUMN))
	{
		self->_name = gtkutil::TreeModel::getSelectedString(treeselection,FULLNAME_COLUMN);
		std::string guiPath = "guis/" + self->_name ;
		self->_editorDialog->updateGuiView( guiPath.c_str() );
	}
	else
		self->_name.clear();
}

} // namespace ui
