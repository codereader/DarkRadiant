#include "FilterDialog.h"

#include "ifilter.h"
#include "iradiant.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include <gtk/gtk.h>

namespace ui {

	namespace {
		const int DEFAULT_SIZE_X = 600;
	    const int DEFAULT_SIZE_Y = 550;
	   	const std::string WINDOW_TITLE = "Filter Settings";

		enum {
			WIDGET_ADD_FILTER_BUTTON,
			WIDGET_DELETE_FILTER_BUTTON,
		};

		enum {
			COL_NAME,
			COL_STATE,
			COL_COLOUR,
			COL_READONLY,
			NUM_COLUMNS
		};
	}

FilterDialog::FilterDialog() :
	BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow()),
	_filterStore(gtk_list_store_new(NUM_COLUMNS, 
									G_TYPE_STRING,		// name
									G_TYPE_STRING,		// state
									G_TYPE_STRING,		// colour
									G_TYPE_BOOLEAN))	// read-only
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Create the child widgets
	populateWindow();

	// Refresh dialog contents
	update();

	// Show the window and its children, enter the main loop
	show();
}

void FilterDialog::save() {
	// TODO
}

void FilterDialog::update() {
	
	// Local helper class to populate the liststore
	class FilterStorePopulator :
		public IFilterVisitor
	{
		GtkListStore* _store;
	public:
		FilterStorePopulator(GtkListStore* targetStore) :
			_store(targetStore)
		{}

		void visit(const std::string& filterName) {
			GtkTreeIter iter;
		
			// Allocate a new list store element and store its pointer into <iter>
			gtk_list_store_append(_store, &iter);
			
			bool state = GlobalFilterSystem().getFilterState(filterName);
			bool readOnly = GlobalFilterSystem().filterIsReadOnly(filterName);
			
			gtk_list_store_set(_store, &iter, COL_NAME, filterName.c_str(), 
											  COL_STATE, state ? "enabled" : "disabled", 
											  COL_COLOUR, readOnly ? "#707070" : "black",
											  COL_READONLY, readOnly ? TRUE : FALSE,
											  -1);
		}

	} populator(_filterStore);

	// Clear the store first
	gtk_list_store_clear(_filterStore);

	// Traverse the filters
	GlobalFilterSystem().forEachFilter(populator);

	// Update the button sensitivity
	updateWidgetSensitivity();
}

void FilterDialog::populateWindow() {
	// Create the dialog vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the "Filters" label	
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Filters</b>"), FALSE, FALSE, 0);

	// Pack the treeview into the main window's vbox
	gtk_box_pack_start(GTK_BOX(vbox), createFiltersPanel(), TRUE, TRUE, 0);

	// Buttons
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), GTK_WIDGET(vbox));
}

GtkWidget* FilterDialog::createFiltersPanel() {
	// Create an hbox for the treeview and the action buttons
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	// Create a new treeview
	_filterView = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_filterStore)));
		
	gtkutil::TextColumn filterCol("Name", COL_NAME);
	gtkutil::TextColumn stateCol("State", COL_STATE);

	gtk_tree_view_column_set_attributes(filterCol, GTK_CELL_RENDERER(filterCol.getCellRenderer()),
										"markup", COL_NAME,
										"foreground", COL_COLOUR,
                                        NULL);

	gtk_tree_view_column_set_attributes(stateCol, GTK_CELL_RENDERER(stateCol.getCellRenderer()),
										"markup", COL_STATE,
										"foreground", COL_COLOUR,
                                        NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_filterView), filterCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_filterView), stateCol);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(_filterView));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onFilterSelectionChanged), this);

	// Action buttons
	_widgets[WIDGET_ADD_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_ADD);
	_widgets[WIDGET_DELETE_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_DELETE);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_ADD_FILTER_BUTTON]), "clicked", G_CALLBACK(onAddFilter), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_FILTER_BUTTON]), "clicked", G_CALLBACK(onDeleteFilter), this);

	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_ADD_FILTER_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_DELETE_FILTER_BUTTON], FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::ScrolledFrame(GTK_WIDGET(_filterView)), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), actionVBox, FALSE, FALSE, 0);

	return gtkutil::LeftAlignment(hbox, 18, 1);
}

GtkWidget* FilterDialog::createButtonPanel() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Save button
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onSave), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
	
	// Cancel Button
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

void FilterDialog::updateWidgetSensitivity() {
	if (!_selectedFilter.empty()) {
		// We have a filter, is it read-only?
		bool readonly = GlobalFilterSystem().filterIsReadOnly(_selectedFilter);
		
		gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_FILTER_BUTTON], readonly ? FALSE : TRUE);
	}
	else {
		// no filter selected
		gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_FILTER_BUTTON], FALSE);
	}
}

void FilterDialog::showDialog() {
	// Instantiate a new instance, blocks GTK
	FilterDialog instance;
}

void FilterDialog::onCancel(GtkWidget* widget, FilterDialog* self) {
	// destroy dialog without saving
	self->destroy();
}

void FilterDialog::onSave(GtkWidget* widget, FilterDialog* self) {
	self->save();
	
	// TODO: Check rebuilding the filter menu

	self->destroy();
}

void FilterDialog::onAddFilter(GtkWidget* w, FilterDialog* self) {
	// TODO
}

void FilterDialog::onDeleteFilter(GtkWidget* w, FilterDialog* self) {
	// Check the prerequisites
	if (self->_selectedFilter.empty() || 
		GlobalFilterSystem().filterIsReadOnly(self->_selectedFilter))
	{
		// No filter selected or read-only
		return;
	}

	GlobalFilterSystem().removeFilter(self->_selectedFilter);

	// Update all widgets
	self->update();
}

void FilterDialog::onFilterSelectionChanged(GtkTreeSelection* sel, FilterDialog* self) {
	// Get the selection
	GtkTreeIter selected;
	bool hasSelection = gtk_tree_selection_get_selected(sel, NULL, &selected) ? true : false;

	if (hasSelection) {
		self->_selectedFilter = gtkutil::TreeModel::getString(
			GTK_TREE_MODEL(self->_filterStore), &selected, COL_NAME
		);
	}
	else {
		self->_selectedFilter = "";
	}

	self->updateWidgetSensitivity();
}

} // namespace ui
