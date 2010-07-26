#include "FilterDialog.h"

#include "i18n.h"
#include "ifilter.h"
#include "imainframe.h"
#include "idialogmanager.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include <gtk/gtk.h>
#include "ui/menu/FiltersMenu.h"

#include "FilterEditor.h"

namespace ui {

	namespace {
		const int DEFAULT_SIZE_X = 600;
	    const int DEFAULT_SIZE_Y = 550;
	   	const char* const WINDOW_TITLE = N_("Filter Settings");

		enum {
			WIDGET_ADD_FILTER_BUTTON,
			WIDGET_EDIT_FILTER_BUTTON,
			WIDGET_VIEW_FILTER_BUTTON,
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
	BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
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

	// Load the filters from the filtersystem
	loadFilters();

	// Refresh dialog contents
	update();

	// Show the window and its children, enter the main loop
	show();
}

void FilterDialog::save() {
	// Delete filters marked for removal
	for (FilterMap::const_iterator i = _deletedFilters.begin(); i != _deletedFilters.end(); ++i) {
		GlobalFilterSystem().removeFilter(i->first);
	}

	// Save all remaining filters
	for (FilterMap::const_iterator i = _filters.begin(); i != _filters.end(); ++i) {
		// Check if the name has changed (or a new filters has been defined)
		if (i->second->nameHasChanged()) {
			// New filters have their original name set to the empty string
			if (i->second->getOriginalName().empty()) {
				// Insert a new filter
				GlobalFilterSystem().addFilter(i->second->name, i->second->rules);
			}
			else {
				// Existing filer, issue the rename command
				GlobalFilterSystem().renameFilter(i->second->getOriginalName(), i->second->name);
			}
		}

		// Save the ruleset (to the new name, in case the filter has been renamed)
		GlobalFilterSystem().setFilterRules(i->first, i->second->rules);
	}

	// Trigger an update
	GlobalFilterSystem().update();

	// Re-build the filters menu
	ui::FiltersMenu::addItemsToMainMenu();
}

void FilterDialog::loadFilters() {
	// Clear first, before population
	_filters.clear();

	// Local helper class to populate the map
	class FilterMapPopulator :
		public IFilterVisitor
	{
		FilterMap& _target;
	public:
		FilterMapPopulator(FilterMap& target) :
			_target(target)
		{}

		void visit(const std::string& filterName) {
			// Get the properties
			bool state = GlobalFilterSystem().getFilterState(filterName);
			bool readOnly = GlobalFilterSystem().filterIsReadOnly(filterName);

			std::pair<FilterMap::iterator, bool> result = _target.insert(
				FilterMap::value_type(filterName, FilterPtr(new Filter(filterName, state, readOnly)))
			);

			// Copy the ruleset from the given filter
			result.first->second->rules = GlobalFilterSystem().getRuleSet(filterName);
		}

	} populator(_filters);

	GlobalFilterSystem().forEachFilter(populator);
}

void FilterDialog::update() {
	// Clear the store first
	gtk_list_store_clear(_filterStore);

	for (FilterMap::const_iterator i = _filters.begin(); i != _filters.end(); ++i) {
		GtkTreeIter iter;
		
		// Allocate a new list store element and store its pointer into <iter>
		gtk_list_store_append(_filterStore, &iter);

		const Filter& filter = *(i->second);
				
		gtk_list_store_set(_filterStore, &iter, COL_NAME, i->first.c_str(), 
										  COL_STATE, filter.state ? _("enabled") : _("disabled"), 
										  COL_COLOUR, filter.readOnly ? "#707070" : "black",
										  COL_READONLY, filter.readOnly ? TRUE : FALSE,
										  -1);
	}

	// Update the button sensitivity
	updateWidgetSensitivity();
}

void FilterDialog::populateWindow() {
	// Create the dialog vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the "Filters" label	
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Filters") + "</b>"), FALSE, FALSE, 0);

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
		
	gtkutil::TextColumn filterCol(_("Name"), COL_NAME);
	gtkutil::TextColumn stateCol(_("State"), COL_STATE);

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
	_widgets[WIDGET_EDIT_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	_widgets[WIDGET_VIEW_FILTER_BUTTON] = gtk_button_new_with_label(_("View"));
		
	_widgets[WIDGET_DELETE_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_DELETE);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_ADD_FILTER_BUTTON]), "clicked", G_CALLBACK(onAddFilter), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_EDIT_FILTER_BUTTON]), "clicked", G_CALLBACK(onEditFilter), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_VIEW_FILTER_BUTTON]), "clicked", G_CALLBACK(onViewFilter), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_FILTER_BUTTON]), "clicked", G_CALLBACK(onDeleteFilter), this);

	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_ADD_FILTER_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_EDIT_FILTER_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_VIEW_FILTER_BUTTON], FALSE, FALSE, 0);
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
		FilterMap::const_iterator i = _filters.find(_selectedFilter);

		if (i != _filters.end()) {

			if (i->second->readOnly) {
				gtk_widget_hide(_widgets[WIDGET_EDIT_FILTER_BUTTON]);
				gtk_widget_show(_widgets[WIDGET_VIEW_FILTER_BUTTON]);
			}
			else {
				gtk_widget_show(_widgets[WIDGET_EDIT_FILTER_BUTTON]);
				gtk_widget_hide(_widgets[WIDGET_VIEW_FILTER_BUTTON]);
			}

			gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_FILTER_BUTTON], i->second->readOnly ? FALSE : TRUE);
			gtk_widget_set_sensitive(_widgets[WIDGET_EDIT_FILTER_BUTTON], i->second->readOnly ? FALSE : TRUE);
			gtk_widget_set_sensitive(_widgets[WIDGET_VIEW_FILTER_BUTTON], i->second->readOnly ? TRUE : FALSE);

			return;
		}
	}

	// no valid filter selected
	gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_FILTER_BUTTON], FALSE);
	gtk_widget_set_sensitive(_widgets[WIDGET_EDIT_FILTER_BUTTON], FALSE);
	gtk_widget_set_sensitive(_widgets[WIDGET_VIEW_FILTER_BUTTON], FALSE);
	gtk_widget_hide(_widgets[WIDGET_EDIT_FILTER_BUTTON]);
	gtk_widget_show(_widgets[WIDGET_VIEW_FILTER_BUTTON]);
}

void FilterDialog::showDialog(const cmd::ArgumentList& args) {
	// Instantiate a new instance, blocks GTK
	FilterDialog instance;
}

void FilterDialog::onCancel(GtkWidget* widget, FilterDialog* self) {
	// destroy dialog without saving
	self->destroy();
}

void FilterDialog::onSave(GtkWidget* widget, FilterDialog* self) {
	// Save changes
	self->save();
	
	// Close the dialog
	self->destroy();
}

void FilterDialog::onAddFilter(GtkWidget* w, FilterDialog* self) {
	// Construct a new filter with an empty name (this indicates it has not been there before when saving)
	FilterPtr workingCopy(new Filter("", false, false));
	workingCopy->name = _("NewFilter");

	// Instantiate a new editor, will block
	FilterEditor editor(*workingCopy, self->getRefPtr(), false);

	if (editor.getResult() != FilterEditor::RESULT_OK) {
		// User hit cancel, we're done
		return;
	}

	if (workingCopy->rules.empty())
	{
		// Empty ruleset, notify user
		IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Empty Filter"),
			_("No rules defined for this filter, cannot insert."), ui::IDialog::MESSAGE_ERROR);

		dialog->run();
		return;
	}

	std::pair<FilterMap::iterator, bool> result = self->_filters.insert(
		FilterMap::value_type(workingCopy->name, workingCopy)
	);

	if (!result.second)
	{
		// Empty ruleset, notify user
		IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Name Conflict"),
			_("Cannot add, filter with same name already exists."), ui::IDialog::MESSAGE_ERROR);

		dialog->run();
		return;
	}

	self->update();
}

void FilterDialog::onViewFilter(GtkWidget* w, FilterDialog* self) {
	// Lookup the Filter object
	FilterMap::iterator f = self->_filters.find(self->_selectedFilter);

	if (f == self->_filters.end()) {
		return; // not found 
	}

	// Construct a new filter
	Filter workingCopy(*(f->second));

	// Instantiate a new editor, will block
	FilterEditor editor(workingCopy, self->getRefPtr(), true);
}

void FilterDialog::onEditFilter(GtkWidget* w, FilterDialog* self) {
	// Lookup the Filter object
	FilterMap::iterator f = self->_filters.find(self->_selectedFilter);

	if (f == self->_filters.end() || f->second->readOnly) {
		return; // not found or read-only
	}

	// Copy-construct a new filter
	Filter workingCopy(*(f->second));

	// Instantiate a new editor, will block
	FilterEditor editor(workingCopy, self->getRefPtr(), false);

	if (editor.getResult() != FilterEditor::RESULT_OK) {
		// User hit cancel, we're done
		return;
	}

	if (workingCopy.rules.empty())
	{
		// Empty ruleset, ask user for deletion
		IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Empty Filter"),
			_("No rules defined for this filter. Delete it?"), ui::IDialog::MESSAGE_ASK);

		if (dialog->run() == IDialog::RESULT_YES)
		{
			// Move the object from _filters to _deletedfilters
			self->_deletedFilters.insert(*f);
			self->_filters.erase(f);
		}
		else {
			// Don't delete the empty filter, leave the old one alone
		}
	}
	else {
		// Ruleset is ok, has the name changed?

		if (workingCopy.name != f->first) {
			// Name has changed, relocate the filter object
			self->_filters.erase(f->first);
			self->_filters[workingCopy.name] = FilterPtr(new Filter(workingCopy));
		}
		else {
			// No name change, just overwrite the filter object
			*(f->second) = workingCopy;
		}
	}

	// Update all widgets
	self->update();
}

void FilterDialog::onDeleteFilter(GtkWidget* w, FilterDialog* self) {
	// Lookup the Filter object
	FilterMap::iterator f = self->_filters.find(self->_selectedFilter);

	if (f == self->_filters.end() || f->second->readOnly) {
		return; // not found or read-only
	}

	// Move the object from _filters to _deletedfilters
	self->_deletedFilters.insert(*f);
	self->_filters.erase(f);

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
