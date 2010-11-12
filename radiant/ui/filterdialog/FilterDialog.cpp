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

#include "ui/menu/FiltersMenu.h"

#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

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
	}

FilterDialog::FilterDialog() :
	BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_filterStore(Gtk::ListStore::create(_columns))
{
	set_default_size(DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create the child widgets
	populateWindow();

	// Load the filters from the filtersystem
	loadFilters();

	// Refresh dialog contents
	update();

	// Show the window and its children, enter the main loop
	show();
}

void FilterDialog::save()
{
	// Delete filters marked for removal
	for (FilterMap::const_iterator i = _deletedFilters.begin(); i != _deletedFilters.end(); ++i)
	{
		GlobalFilterSystem().removeFilter(i->first);
	}

	// Save all remaining filters
	for (FilterMap::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
	{
		// Check if the name has changed (or a new filters has been defined)
		if (i->second->nameHasChanged())
		{
			// New filters have their original name set to the empty string
			if (i->second->getOriginalName().empty())
			{
				// Insert a new filter
				GlobalFilterSystem().addFilter(i->second->name, i->second->rules);
			}
			else
			{
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

void FilterDialog::loadFilters()
{
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

		void visit(const std::string& filterName)
		{
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

void FilterDialog::update()
{
	// Clear the store first
	_filterStore->clear();

	for (FilterMap::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
	{
		const Filter& filter = *(i->second);

		Gtk::TreeModel::Row row = *_filterStore->append();

		row[_columns.name] = i->first;
		row[_columns.state] = filter.state ? std::string(_("enabled")) : std::string(_("disabled"));
		row[_columns.colour] = filter.readOnly ? std::string("#707070") : std::string("black");
		row[_columns.readonly] = filter.readOnly;
	}

	// Update the button sensitivity
	updateWidgetSensitivity();
}

void FilterDialog::populateWindow()
{
	// Create the dialog vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the "Filters" label
	vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<b>") + _("Filters") + "</b>")), false, false, 0);

	// Pack the treeview into the main window's vbox
	vbox->pack_start(createFiltersPanel(), true, true, 0);

	// Buttons
	vbox->pack_start(createButtonPanel(), false, false, 0);

	add(*vbox);
}

Gtk::Widget& FilterDialog::createFiltersPanel()
{
	// Create an hbox for the treeview and the action buttons
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	// Create a new treeview
	_filterView = Gtk::manage(new Gtk::TreeView(_filterStore));

	Gtk::TreeViewColumn* filterCol = Gtk::manage(
		new gtkutil::ColouredTextColumn(_("Name"), _columns.name, _columns.colour)
	);

	Gtk::TreeViewColumn* stateCol = Gtk::manage(
		new gtkutil::ColouredTextColumn(_("State"), _columns.state, _columns.colour)
	);

	_filterView->append_column(*filterCol);
	_filterView->append_column(*stateCol);

	Glib::RefPtr<Gtk::TreeSelection> sel = _filterView->get_selection();
	sel->signal_changed().connect(sigc::mem_fun(*this, &FilterDialog::onFilterSelectionChanged));

	// Action buttons
	Gtk::Button* addFilterButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	Gtk::Button* editFilterButton = Gtk::manage(new Gtk::Button(Gtk::Stock::EDIT));
	Gtk::Button* viewFilterButton = Gtk::manage(new Gtk::Button(_("View")));
	Gtk::Button* deleteFilterButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));

	_widgets[WIDGET_ADD_FILTER_BUTTON] = addFilterButton;
	_widgets[WIDGET_EDIT_FILTER_BUTTON] = editFilterButton;
	_widgets[WIDGET_VIEW_FILTER_BUTTON] = viewFilterButton;

	_widgets[WIDGET_DELETE_FILTER_BUTTON] = deleteFilterButton;

	addFilterButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterDialog::onAddFilter));
	editFilterButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterDialog::onEditFilter));
	viewFilterButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterDialog::onViewFilter));
	deleteFilterButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterDialog::onDeleteFilter));

	Gtk::VBox* actionVBox = Gtk::manage(new Gtk::VBox(false, 6));

	actionVBox->pack_start(*_widgets[WIDGET_ADD_FILTER_BUTTON], false, false, 0);
	actionVBox->pack_start(*_widgets[WIDGET_EDIT_FILTER_BUTTON], false, false, 0);
	actionVBox->pack_start(*_widgets[WIDGET_VIEW_FILTER_BUTTON], false, false, 0);
	actionVBox->pack_start(*_widgets[WIDGET_DELETE_FILTER_BUTTON], false, false, 0);

	hbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_filterView)), true, true, 0);
	hbox->pack_start(*actionVBox, false, false, 0);

	return *Gtk::manage(new gtkutil::LeftAlignment(*hbox, 18, 1));
}

Gtk::Widget& FilterDialog::createButtonPanel()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterDialog::onSave));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &FilterDialog::onCancel));

	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

void FilterDialog::updateWidgetSensitivity()
{
	if (!_selectedFilter.empty())
	{
		// We have a filter, is it read-only?
		FilterMap::const_iterator i = _filters.find(_selectedFilter);

		if (i != _filters.end()) {

			if (i->second->readOnly) {
				_widgets[WIDGET_EDIT_FILTER_BUTTON]->hide();
				_widgets[WIDGET_VIEW_FILTER_BUTTON]->show();
			}
			else {
				_widgets[WIDGET_EDIT_FILTER_BUTTON]->show();
				_widgets[WIDGET_VIEW_FILTER_BUTTON]->hide();
			}

			_widgets[WIDGET_DELETE_FILTER_BUTTON]->set_sensitive(!i->second->readOnly);
			_widgets[WIDGET_EDIT_FILTER_BUTTON]->set_sensitive(!i->second->readOnly);
			_widgets[WIDGET_VIEW_FILTER_BUTTON]->set_sensitive(i->second->readOnly);
			return;
		}
	}

	// no valid filter selected
	_widgets[WIDGET_DELETE_FILTER_BUTTON]->set_sensitive(false);
	_widgets[WIDGET_EDIT_FILTER_BUTTON]->set_sensitive(false);
	_widgets[WIDGET_VIEW_FILTER_BUTTON]->set_sensitive(false);

	_widgets[WIDGET_EDIT_FILTER_BUTTON]->hide();
	_widgets[WIDGET_VIEW_FILTER_BUTTON]->show();
}

void FilterDialog::showDialog(const cmd::ArgumentList& args)
{
	// Instantiate a new instance, blocks GTK
	FilterDialog instance;
}

void FilterDialog::onCancel()
{
	// destroy dialog without saving
	destroy();
}

void FilterDialog::onSave()
{
	// Save changes
	save();

	// Close the dialog
	destroy();
}

void FilterDialog::onAddFilter()
{
	// Construct a new filter with an empty name (this indicates it has not been there before when saving)
	FilterPtr workingCopy(new Filter("", false, false));
	workingCopy->name = _("NewFilter");

	// Instantiate a new editor, will block
	FilterEditor editor(*workingCopy, getRefPtr(), false);

	if (editor.getResult() != FilterEditor::RESULT_OK)
	{
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

	std::pair<FilterMap::iterator, bool> result = _filters.insert(
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

	update();
}

void FilterDialog::onViewFilter()
{
	// Lookup the Filter object
	FilterMap::iterator f = _filters.find(_selectedFilter);

	if (f == _filters.end()) {
		return; // not found
	}

	// Construct a new filter
	Filter workingCopy(*(f->second));

	// Instantiate a new editor, will block
	FilterEditor editor(workingCopy, getRefPtr(), true);
}

void FilterDialog::onEditFilter()
{
	// Lookup the Filter object
	FilterMap::iterator f = _filters.find(_selectedFilter);

	if (f == _filters.end() || f->second->readOnly) {
		return; // not found or read-only
	}

	// Copy-construct a new filter
	Filter workingCopy(*(f->second));

	// Instantiate a new editor, will block
	FilterEditor editor(workingCopy, getRefPtr(), false);

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
			_deletedFilters.insert(*f);
			_filters.erase(f);
		}
		else {
			// Don't delete the empty filter, leave the old one alone
		}
	}
	else {
		// Ruleset is ok, has the name changed?

		if (workingCopy.name != f->first) {
			// Name has changed, relocate the filter object
			_filters.erase(f->first);
			_filters[workingCopy.name] = FilterPtr(new Filter(workingCopy));
		}
		else {
			// No name change, just overwrite the filter object
			*(f->second) = workingCopy;
		}
	}

	// Update all widgets
	update();
}

void FilterDialog::onDeleteFilter()
{
	// Lookup the Filter object
	FilterMap::iterator f = _filters.find(_selectedFilter);

	if (f == _filters.end() || f->second->readOnly) {
		return; // not found or read-only
	}

	// Move the object from _filters to _deletedfilters
	_deletedFilters.insert(*f);
	_filters.erase(f);

	// Update all widgets
	update();
}

void FilterDialog::onFilterSelectionChanged()
{
	Gtk::TreeModel::iterator iter = _filterView->get_selection()->get_selected();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		_selectedFilter = Glib::ustring(row[_columns.name]);
	}
	else
	{
		_selectedFilter.clear();
	}

	updateWidgetSensitivity();
}

} // namespace ui
