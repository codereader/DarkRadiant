#include "ComponentsDialog.h"
#include "Objective.h"
#include "ce/ComponentEditorFactory.h"

#include "itextstream.h"
#include "string/convert.h"

#include "i18n.h"
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>

namespace objectives
{

/* CONSTANTS */

namespace
{

const char* const DIALOG_TITLE = N_("Edit Objective");

} // namespace

// Main constructor
ComponentsDialog::ComponentsDialog(wxWindow* parent, Objective& objective) :
	DialogBase(_(DIALOG_TITLE), parent),
	_objective(objective),
	_componentList(new wxutil::TreeModel(_columns, true)),
	_components(objective.components), // copy the components to our local working set
	_updateMutex(false),
	_timer(500, _onIntervalReached, this)
{
	wxPanel* mainPanel = loadNamedPanel(this, "ObjCompMainPanel");

	// Dialog contains list view, edit panel and buttons
	setupObjectiveEditPanel();

	makeLabelBold(this, "ObjCompListLabel");

	createListView();
	setupEditPanel();

	// Populate the list of components
	populateObjectiveEditPanel();
	populateComponents();
	
	mainPanel->Layout();
	mainPanel->Fit();
	Fit();
	CenterOnParent();
}

ComponentsDialog::~ComponentsDialog()
{
	_timer.disable();
}

// Create the panel for editing the currently-selected objective
void ComponentsDialog::setupObjectiveEditPanel()
{
	_objDescriptionEntry = findNamedObject<wxTextCtrl>(this, "ObjCompDescription");
	_objStateCombo = findNamedObject<wxChoice>(this, "ObjCompInitialState");

	_diffPanel.reset(new DifficultyPanel(findNamedObject<wxPanel>(this, "ObjCompDiffPanel")));

	// Populate the list of states. This must be done in order to match the
	// values in the enum, since the index will be used when writing to entity
	_objStateCombo->Append(Objective::getStateText(Objective::INCOMPLETE), 
		new wxStringClientData(string::to_string(Objective::INCOMPLETE)));
	_objStateCombo->Append(Objective::getStateText(Objective::COMPLETE),
		new wxStringClientData(string::to_string(Objective::COMPLETE)));
	_objStateCombo->Append(Objective::getStateText(Objective::INVALID),
		new wxStringClientData(string::to_string(Objective::INVALID)));
	_objStateCombo->Append(Objective::getStateText(Objective::FAILED),
		new wxStringClientData(string::to_string(Objective::FAILED)));

	_objMandatoryFlag = findNamedObject<wxCheckBox>(this, "ObjCompObjMandatory");
	_objIrreversibleFlag = findNamedObject<wxCheckBox>(this, "ObjCompObjIrreversible");
	_objOngoingFlag = findNamedObject<wxCheckBox>(this, "ObjCompObjOngoing");
	_objVisibleFlag = findNamedObject<wxCheckBox>(this, "ObjCompObjVisible");

	// Enabling objectives
	_enablingObjs = findNamedObject<wxTextCtrl>(this, "ObjCompEnablingObjectives");
	
	// Logic
	_successLogic = findNamedObject<wxTextCtrl>(this, "ObjCompSuccessLogic");
	_failureLogic = findNamedObject<wxTextCtrl>(this, "ObjCompFailureLogic");

	// Completion/failure scripts
	_completionScript = findNamedObject<wxTextCtrl>(this, "ObjCompCompletionScript");
	_failureScript = findNamedObject<wxTextCtrl>(this, "ObjCompFailureScript");

	// Completion/failure targets
	_completionTarget = findNamedObject<wxTextCtrl>(this, "ObjCompCompletionTarget");
	_failureTarget = findNamedObject<wxTextCtrl>(this, "ObjCompFailureTarget");
}

// Create list view
void ComponentsDialog::createListView()
{
	// Create tree view and connect selection changed callback
	wxPanel* treeViewPanel = findNamedObject<wxPanel>(this, "ObjCompListViewPanel");
	_componentView = wxutil::TreeView::CreateWithModel(treeViewPanel, _componentList);
	treeViewPanel->GetSizer()->Add(_componentView, 1, wxEXPAND);

	_componentView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ComponentsDialog::_onSelectionChanged), NULL, this);

	_componentView->AppendTextColumn("#", _columns.index.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_componentView->AppendTextColumn(_("Type"), _columns.description.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Create Add and Delete buttons for components
	wxButton* addButton = findNamedObject<wxButton>(this, "ObjCompAddComponentButton");
	wxButton* delButton = findNamedObject<wxButton>(this, "ObjCompDeleteComponentButton");

	addButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ComponentsDialog::_onAddComponent), NULL, this);
	delButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ComponentsDialog::_onDeleteComponent), NULL, this);
}

// Create edit panel
void ComponentsDialog::setupEditPanel()
{
	// Table
	_editPanel = findNamedObject<wxPanel>(this, "ObjCompComponentEditPanel");

	// Component type dropdown
	_typeCombo = findNamedObject<wxChoice>(this, "ObjCompComponentType");
	_typeCombo->Connect(wxEVT_CHOICE, wxCommandEventHandler(ComponentsDialog::_onTypeChanged), NULL, this);

	// Populate the combo box. The set is in ID order.
	for (ComponentTypeSet::const_iterator i = ComponentType::SET_ALL().begin();
		 i != ComponentType::SET_ALL().end();
		 ++i)
	{
		_typeCombo->Append(i->getDisplayName(), new wxStringClientData(string::to_string(i->getId())));
	}

	// Flags hbox
	_stateFlag = findNamedObject<wxCheckBox>(this, "ObjCompSatisfiedAtStart");
	_irreversibleFlag = findNamedObject<wxCheckBox>(this, "ObjCompIrreversible");
	_invertedFlag = findNamedObject<wxCheckBox>(this, "ObjCompBooleanNOT");
	_playerResponsibleFlag = findNamedObject<wxCheckBox>(this, "ObjCompPlayerResponsible");

	_stateFlag->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(ComponentsDialog::_onCompToggleChanged), NULL, this);
	_irreversibleFlag->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(ComponentsDialog::_onCompToggleChanged), NULL, this);
	_invertedFlag->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(ComponentsDialog::_onCompToggleChanged), NULL, this);
	_playerResponsibleFlag->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(ComponentsDialog::_onCompToggleChanged), NULL, this);

	_compEditorPanel = findNamedObject<wxPanel>(this, "ObjCompEditorContainer");
}

// Populate the component list
void ComponentsDialog::populateComponents()
{
	// Clear the list store
	_componentList->clear();

	// Add components from the Objective to the list store
	for (Objective::ComponentMap::iterator i = _components.begin();
		 i != _components.end();
		 ++i)
	{
		Gtk::TreeModel::Row row = *_componentList->append();

		row[_columns.index] = i->first;
		row[_columns.description] = i->second.getString();
	}
}

void ComponentsDialog::updateComponents()
{
	// Traverse all components and update the items in the list
	for (Objective::ComponentMap::iterator i = _components.begin();
		 i != _components.end();
		 ++i)
	{
		// Find the item in the list store (0th column carries the ID)
		gtkutil::TreeModel::SelectionFinder finder(i->first, _columns.index.index());

		// Traverse the model
		_componentList->foreach_iter(sigc::mem_fun(finder, &gtkutil::TreeModel::SelectionFinder::forEach));

		// Check if we found the item
		if (finder.getIter())
		{
			Gtk::TreeModel::Row row = *finder.getIter();

			row[_columns.index] = i->first;
			row[_columns.description] = i->second.getString();
		}
	}
}

// Populate the edit panel
void ComponentsDialog::populateEditPanel(int index)
{
	// Get the component
	Component& comp = _components[index];

	// Set the flags
	_stateFlag->set_active(comp.isSatisfied());
	_irreversibleFlag->set_active(comp.isIrreversible());
	_invertedFlag->set_active(comp.isInverted());
	_playerResponsibleFlag->set_active(comp.isPlayerResponsible());

	// Change the type combo if necessary.
	// Since the combo box was populated in
	// ID order, we can simply use our ComponentType's ID as an index.
    if (_typeCombo->get_active_row_number() != comp.getType().getId())
    {
		// Change the combo selection (this triggers a change of the
        // ComponentEditor panel)
		_typeCombo->set_active(comp.getType().getId());
    }
    else
    {
        // Update the ComponentEditor ourselves, since the new Component has the
        // same type but we still want to refresh the panel with the new
        // contents
        changeComponentEditor(comp);
    }
}

// Populate the edit panel widgets using the given objective number
void ComponentsDialog::populateObjectiveEditPanel()
{
	// Disable GTK callbacks while we're at it
	_updateMutex = true;

	// Get the objective
	const Objective& obj = _objective;

	// Set description text
	_objDescriptionEntry->set_text(obj.description);

	// Update the difficulty panel
	_diffPanel->populateFromObjective(obj);

	// Set initial state enum
	_objStateCombo->set_active(static_cast<int>(obj.state));

	// Set flags
	_objIrreversibleFlag->set_active(obj.irreversible);
	_objOngoingFlag->set_active(obj.ongoing);
	_objMandatoryFlag->set_active(obj.mandatory);
	_objVisibleFlag->set_active(obj.visible);

	_enablingObjs->set_text(obj.enablingObjs);

	_successLogic->set_text(obj.logic.successLogic);
	_failureLogic->set_text(obj.logic.failureLogic);

	_completionScript->set_text(obj.completionScript);
	_failureScript->set_text(obj.failureScript);

	_completionTarget->set_text(obj.completionTarget);
	_failureTarget->set_text(obj.failureTarget);

	_updateMutex = false;
}

// Get selected component index
int ComponentsDialog::getSelectedIndex()
{
	// Get the selection if valid
	Gtk::TreeModel::iterator iter = _componentView->get_selection()->get_selected();

	if (iter)
	{
		// Valid selection, return the contents of the index column
		return (*iter)[_columns.index];
	}
	else
	{
		return -1;
	}
}

// Change component editor
void ComponentsDialog::changeComponentEditor(Component& compToEdit)
{
	// greebo: Get a new component editor, any previous one will auto-destroy and
	// remove its widget from the container.
	_componentEditor = ce::ComponentEditorFactory::create(
        compToEdit.getType().getName(), compToEdit
	);

	if (_componentEditor != NULL)
	{
		// Get the widget from the ComponentEditor and show it
		// Pack the widget into the containing frame
		_compEditorPanel->add(*_componentEditor->getWidget());
		_compEditorPanel->show_all();
	}
}

// Safely write the ComponentEditor contents to the Component
void ComponentsDialog::checkWriteComponent()
{
	if (_componentEditor)
	{
        _componentEditor->writeToComponent();
	}
}

void ComponentsDialog::save()
{
	// Write the objective properties
	_objective.description = _objDescriptionEntry->get_text();

	// Save the difficulty settings
	_diffPanel->writeToObjective(_objective);

	// Set the initial state enum value from the combo box index
	_objective.state = static_cast<Objective::State>(_objStateCombo->get_active_row_number());

	// Determine which checkbox is toggled, then update the appropriate flag
	_objective.mandatory = _objMandatoryFlag->get_active();
	_objective.visible = _objVisibleFlag->get_active();
	_objective.ongoing = _objOngoingFlag->get_active();
	_objective.irreversible = _objIrreversibleFlag->get_active();

	// Enabling objectives
	_objective.enablingObjs = _enablingObjs->get_text();

	// Success/failure logic
	_objective.logic.successLogic = _successLogic->get_text();
	_objective.logic.failureLogic = _failureLogic->get_text();

	// Completion/Failure script
	_objective.completionScript = _completionScript->get_text();
	_objective.failureScript = _failureScript->get_text();

	// Completion/Failure targets
	_objective.completionTarget = _completionTarget->get_text();
	_objective.failureTarget = _failureTarget->get_text();

	// Write the components
	checkWriteComponent();

	// Copy the working set over the ones in the objective
	_objective.components.swap(_components);
}

// Save button
void ComponentsDialog::_onOK()
{
	save();
	destroy();
}

// Cancel button
void ComponentsDialog::_onCancel()
{
	// Destroy the dialog without saving
    destroy();
}

void ComponentsDialog::_onCompToggleChanged(Gtk::CheckButton* button)
{
	if (_updateMutex) return;

	// Update the Component working copy. The selected index must be valid, since the
	// edit panel is only sensitive if a component is selected
	int idx = getSelectedIndex();
	assert(idx >= 0);

	Component& comp = _components[idx];

	if (button == _stateFlag)
	{
		comp.setSatisfied(button->get_active());
	}
	else if (button == _irreversibleFlag)
	{
		comp.setIrreversible(button->get_active());
	}
	else if (button == _invertedFlag)
	{
		comp.setInverted(button->get_active());
	}
	else if (button == _playerResponsibleFlag)
	{
		comp.setPlayerResponsible(button->get_active());
	}

	// Update the list store
	updateComponents();
}

// Selection changed
void ComponentsDialog::_onSelectionChanged()
{
    // Save the existing ComponentEditor contents if req'd
    checkWriteComponent();

	// Get the selection if valid
	Gtk::TreeModel::iterator iter = _componentView->get_selection()->get_selected();

	if (!iter)
    {
		// Disable the edit panel and remove the ComponentEditor
		_compEditorPanel->set_sensitive(false);
		_componentEditor = objectives::ce::ComponentEditorPtr();

		// Turn off the periodic update of the component list
		_timer.disable();
	}
	else
	{
		// Otherwise populate edit panel with the current component index
		int component = (*iter)[_columns.index];

		populateEditPanel(component);

		// Enable the edit panel
		_compEditorPanel->set_sensitive(true);
		_editPanel->set_sensitive(true);

		// Turn on the periodic update
		_timer.enable();
	}
}

// Add a new component
void ComponentsDialog::_onAddComponent()
{
	Objective::ComponentMap& components = _components;

	// Find an unused component number (starting from 1)
	for (int idx = 1; idx < INT_MAX; ++idx)
	{
		if (components.find(idx) == components.end()) {
			// Unused, add a new component here
			Component comp;
			components.insert(std::make_pair(idx, comp));
			break;
		}
	}

	// Refresh the component list
	populateComponents();
}

// Remove a component
void ComponentsDialog::_onDeleteComponent()
{
	// Delete the selected component
	int idx = getSelectedIndex();

	if (idx != -1)
    {
        // Remove the selection first, so our selection-changed callback does not
        // attempt to writeToComponent() after the Component has already been deleted
		_componentView->get_selection()->unselect_all();

        // Erase the actual component
		_components.erase(idx);
	}

	// Refresh the list
	populateComponents();
}

// Type combo changed
void ComponentsDialog::_onTypeChanged()
{
	// Get the current selection
	Gtk::TreeModel::iterator iter = _typeCombo->get_active();

	std::string selectedText;

	if (iter)
	{
		iter->get_value(1, selectedText); // get the string from the first column
	}

	// Update the Objective object. The selected index must be valid, since the
	// edit panel is only sensitive if a component is selected
	int idx = getSelectedIndex();
	assert(idx >= 0);

	Component& comp = _components[idx];

    // Store the newly-selected type in the Component
	comp.setType(ComponentType::getComponentType(selectedText));

    // Change the ComponentEditor
    changeComponentEditor(comp);

	// Update the components list with the new display string
	Gtk::TreeModel::iterator compIter = _componentView->get_selection()->get_selected();

	(*compIter)[_columns.description] = comp.getString();
}

gboolean ComponentsDialog::_onIntervalReached(gpointer data)
{
	ComponentsDialog* self = reinterpret_cast<ComponentsDialog*>(data);

	if (self->_updateMutex) return true;

	self->checkWriteComponent();
	self->updateComponents();

	// Return true, so that the timer gets called again
	return true;
}

} // namespace objectives
