#include "FilterDialog.h"

#include "i18n.h"
#include "ifilter.h"
#include "imainframe.h"
#include "idialogmanager.h"

#include "ui/menu/FiltersMenu.h"

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/button.h>

#include "FilterEditor.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Filter Settings");

	enum Buttons
	{
		WIDGET_ADD_FILTER_BUTTON,
		WIDGET_EDIT_FILTER_BUTTON,
		WIDGET_VIEW_FILTER_BUTTON, 
		WIDGET_DELETE_FILTER_BUTTON,
	};
}

FilterDialog::FilterDialog() :
	DialogBase(_(WINDOW_TITLE)),
	_filterStore(new wxutil::TreeModel(_columns, true))
{
	// Create the child widgets
	populateWindow();

	// Load the filters from the filtersystem
	loadFilters();

	// Refresh dialog contents
	update();

	FitToScreen(0.4f, 0.6f);
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
	FiltersMenu::addItemsToMainMenu();
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
	_filterStore->Clear();

	wxDataViewItemAttr black;
	black.SetColour(wxColor(0,0,0));

	wxDataViewItemAttr grey;
	grey.SetColour(wxColor(112,112,112));

	for (FilterMap::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
	{
		const Filter& filter = *(i->second);

		wxutil::TreeModel::Row row = _filterStore->AddItem();

		row[_columns.name] = i->first;
		row[_columns.state] = filter.state ? std::string(_("enabled")) : std::string(_("disabled"));

		row[_columns.name] = filter.readOnly ? grey : black;
		row[_columns.state] = filter.readOnly ? grey : black;

		row[_columns.readonly] = filter.readOnly;

		row.SendItemAdded();
	}

	// Update the button sensitivity
	updateWidgetSensitivity();
}

void FilterDialog::populateWindow()
{
	loadNamedPanel(this, "FilterDialogMainPanel");

	wxStaticText* label = findNamedObject<wxStaticText>(this, "FilterDialogTopLabel");
	label->SetFont(label->GetFont().Bold());

	// Pack the treeview into the main window's vbox
	createFiltersPanel();

	wxButton* okButton = findNamedObject<wxButton>(this, "FilterDialogOkButton");
	wxButton* cancelButton = findNamedObject<wxButton>(this, "FilterDialogCancelButton");
	
	okButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterDialog::onSave), NULL, this);
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(FilterDialog::onCancel), NULL, this);
}

void FilterDialog::createFiltersPanel()
{
	wxPanel* parent = findNamedObject<wxPanel>(this, "FilterDialogTreeViewPanel");

	// Create a new treeview
	_filterView = wxutil::TreeView::CreateWithModel(parent, _filterStore);

	_filterView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(FilterDialog::onFilterSelectionChanged), NULL, this);

	parent->GetSizer()->Add(_filterView, 1, wxEXPAND | wxLEFT, 12);

		// Display name column with icon
		_filterView->AppendTextColumn(_("Name"), _columns.name.getColumnIndex(), 
			wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

		_filterView->AppendTextColumn(_("State"), _columns.state.getColumnIndex(), 
			wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Action buttons
	_buttons[WIDGET_ADD_FILTER_BUTTON] = findNamedObject<wxButton>(this, "FilterDialogAddButton");
	_buttons[WIDGET_EDIT_FILTER_BUTTON] = findNamedObject<wxButton>(this, "FilterDialogEditButton");;
	_buttons[WIDGET_VIEW_FILTER_BUTTON] = findNamedObject<wxButton>(this, "FilterDialogViewButton");;
	_buttons[WIDGET_DELETE_FILTER_BUTTON] = findNamedObject<wxButton>(this, "FilterDialogDeleteButton");;

	_buttons[WIDGET_ADD_FILTER_BUTTON]->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FilterDialog::onAddFilter), NULL, this);
	_buttons[WIDGET_EDIT_FILTER_BUTTON]->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FilterDialog::onEditFilter), NULL, this);
	_buttons[WIDGET_VIEW_FILTER_BUTTON]->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FilterDialog::onViewFilter), NULL, this);
	_buttons[WIDGET_DELETE_FILTER_BUTTON]->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FilterDialog::onDeleteFilter), NULL, this);
}

void FilterDialog::updateWidgetSensitivity()
{
	if (!_selectedFilter.empty())
	{
		// We have a filter, is it read-only?
		FilterMap::const_iterator i = _filters.find(_selectedFilter);

		if (i != _filters.end())
		{
			_buttons[WIDGET_EDIT_FILTER_BUTTON]->Show(!i->second->readOnly);
			_buttons[WIDGET_VIEW_FILTER_BUTTON]->Show(i->second->readOnly);

			_buttons[WIDGET_EDIT_FILTER_BUTTON]->GetContainingSizer()->Layout();
			
			_buttons[WIDGET_DELETE_FILTER_BUTTON]->Enable(!i->second->readOnly);
			_buttons[WIDGET_EDIT_FILTER_BUTTON]->Enable(!i->second->readOnly);
			_buttons[WIDGET_VIEW_FILTER_BUTTON]->Enable(i->second->readOnly);

			return;
		}
	}

	// no valid filter selected
	_buttons[WIDGET_DELETE_FILTER_BUTTON]->Enable(false);
	_buttons[WIDGET_EDIT_FILTER_BUTTON]->Enable(false);
	_buttons[WIDGET_VIEW_FILTER_BUTTON]->Enable(false);

	_buttons[WIDGET_EDIT_FILTER_BUTTON]->Hide();
	_buttons[WIDGET_VIEW_FILTER_BUTTON]->Show();

	_buttons[WIDGET_EDIT_FILTER_BUTTON]->GetContainingSizer()->Layout();	
}

void FilterDialog::ShowDialog(const cmd::ArgumentList& args)
{
	// Instantiate a new instance, blocks GTK
	FilterDialog* instance = new FilterDialog;

	instance->ShowModal();
	instance->Destroy();
}

void FilterDialog::onCancel(wxCommandEvent& ev)
{
	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void FilterDialog::onSave(wxCommandEvent& ev)
{
	// Save changes
	save();

	// Close the dialog
	EndModal(wxID_OK);
}

void FilterDialog::onAddFilter(wxCommandEvent& ev)
{
	// Construct a new filter with an empty name (this indicates it has not been there before when saving)
	FilterPtr workingCopy(new Filter("", false, false));
	workingCopy->name = _("NewFilter");

	// Instantiate a new editor, will block
	FilterEditor* editor = new FilterEditor(*workingCopy, this, false);

	int editorResult = editor->ShowModal();
	
	editor->Destroy();

	if (editorResult != wxID_OK)
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

void FilterDialog::onViewFilter(wxCommandEvent& ev)
{
	// Lookup the Filter object
	FilterMap::iterator f = _filters.find(_selectedFilter);

	if (f == _filters.end()) {
		return; // not found
	}

	// Construct a new filter
	Filter workingCopy(*(f->second));

	// Instantiate a new editor
	FilterEditor* editor = new FilterEditor(workingCopy, this, true);
	editor->ShowModal();
	editor->Destroy();
}

void FilterDialog::onEditFilter(wxCommandEvent& ev)
{
	// Lookup the Filter object
	FilterMap::iterator f = _filters.find(_selectedFilter);

	if (f == _filters.end() || f->second->readOnly) {
		return; // not found or read-only
	}

	// Copy-construct a new filter
	Filter workingCopy(*(f->second));

	// Instantiate a new editor
	FilterEditor* editor = new FilterEditor(workingCopy, this, false);

	int editorResult = editor->ShowModal();

	editor->Destroy();

	if (editorResult != wxID_OK)
	{
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
		else
		{
			// Don't delete the empty filter, leave the old one alone
		}
	}
	else
	{
		// Ruleset is ok, has the name changed?
		if (workingCopy.name != f->first)
		{
			// Name has changed, relocate the filter object
			_filters.erase(f->first);
			_filters[workingCopy.name] = FilterPtr(new Filter(workingCopy));
		}
		else
		{
			// No name change, just overwrite the filter object
			*(f->second) = workingCopy;
		}
	}

	// Update all widgets
	update();
}

void FilterDialog::onDeleteFilter(wxCommandEvent& ev)
{
	// Lookup the Filter object
	FilterMap::iterator f = _filters.find(_selectedFilter);

	if (f == _filters.end() || f->second->readOnly)
	{
		return; // not found or read-only
	}

	// Move the object from _filters to _deletedfilters
	_deletedFilters.insert(*f);
	_filters.erase(f);

	// Update all widgets
	update();
}

void FilterDialog::onFilterSelectionChanged(wxDataViewEvent& ev)
{
	wxDataViewItem item = _filterView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_filterStore);
		_selectedFilter = row[_columns.name];
	}
	else
	{
		_selectedFilter.clear();
	}

	updateWidgetSensitivity();
}

} // namespace ui
