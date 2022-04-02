#include "ComponentsDialog.h"
#include "Objective.h"
#include "ce/ComponentEditorFactory.h"

#include "itextstream.h"
#include "string/convert.h"

#include "i18n.h"
#include <vector>

#include "wxutil/ChoiceHelper.h"
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
	_updateNeeded(false)
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
	_componentView = wxutil::TreeView::CreateWithModel(treeViewPanel, _componentList.get());
	treeViewPanel->GetSizer()->Add(_componentView, 1, wxEXPAND);
	treeViewPanel->SetMinClientSize(wxSize(-1, 90));

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
	_editPanel->Enable(false); // disabled at start

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
	_componentList->Clear();

	// Add components from the Objective to the list store
	for (Objective::ComponentMap::iterator i = _components.begin();
		 i != _components.end();
		 ++i)
	{
		wxutil::TreeModel::Row row = _componentList->AddItem();

		row[_columns.index] = i->first;
		row[_columns.description] = i->second.getString();

		row.SendItemAdded();
	}

	_updateNeeded = true;
}

void ComponentsDialog::updateComponents()
{
	// Traverse all components and update the items in the list
	for (Objective::ComponentMap::iterator i = _components.begin();
		 i != _components.end();
		 ++i)
	{
		// Find the item in the list store (0th column carries the ID)
		wxDataViewItem item = _componentList->FindInteger(i->first, _columns.index);

		// Check if we found the item
		if (item.IsOk())
		{
			wxutil::TreeModel::Row row(item, *_componentList);

			row[_columns.index] = i->first;
			row[_columns.description] = i->second.getString();

			row.SendItemChanged();
		}
	}
}

// Populate the edit panel
void ComponentsDialog::populateEditPanel(int index)
{
	// Get the component
	Component& comp = _components[index];

	// Set the flags
	_stateFlag->SetValue(comp.isSatisfied());
	_irreversibleFlag->SetValue(comp.isIrreversible());
	_invertedFlag->SetValue(comp.isInverted());
	_playerResponsibleFlag->SetValue(comp.isPlayerResponsible());

	// Change the type combo if necessary.
	// Since the combo box was populated in
	// ID order, we can simply use our ComponentType's ID as an index.
	if (_typeCombo->GetSelection() != comp.getType().getId())
    {
		// Change the combo selection
		_typeCombo->Select(comp.getType().getId());

		// Trigger component change, to update editor panel
		handleTypeChange();
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
	// Disable callbacks while we're at it
	_updateMutex = true;

	// Get the objective
	const Objective& obj = _objective;

	// Set description text
	_objDescriptionEntry->SetValue(obj.description);

	// Update the difficulty panel
	_diffPanel->populateFromObjective(obj);

	// Set initial state enum
	wxutil::ChoiceHelper::SelectItemByStoredId(_objStateCombo, static_cast<int>(obj.state));

	// Set flags
	_objIrreversibleFlag->SetValue(obj.irreversible);
	_objOngoingFlag->SetValue(obj.ongoing);
	_objMandatoryFlag->SetValue(obj.mandatory);
	_objVisibleFlag->SetValue(obj.visible);

	_enablingObjs->SetValue(obj.enablingObjs);

	_successLogic->SetValue(obj.logic.successLogic);
	_failureLogic->SetValue(obj.logic.failureLogic);

	_completionScript->SetValue(obj.completionScript);
	_failureScript->SetValue(obj.failureScript);

	_completionTarget->SetValue(obj.completionTarget);
	_failureTarget->SetValue(obj.failureTarget);

	_updateMutex = false;
}

// Get selected component index
int ComponentsDialog::getSelectedIndex()
{
	// Get the selection if valid
	wxDataViewItem item = _componentView->GetSelection();

	if (item.IsOk())
	{
		// Valid selection, return the contents of the index column
		wxutil::TreeModel::Row row(item, *_componentList);
		return row[_columns.index].getInteger();
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
        _compEditorPanel, compToEdit.getType().getName(), compToEdit
	);

	if (_componentEditor)
	{
        // The widgets are constructed and filled with correct values, 
        // from this point on we can write updates to the component automatically
        _componentEditor->setActive(true);

		// Get the widget from the ComponentEditor and show it
		// Pack the widget into the containing frame
		_compEditorPanel->GetSizer()->Add(_componentEditor->getWidget(), 1, wxEXPAND | wxALL, 12);

        _compEditorPanel->Layout();
		_compEditorPanel->Fit();
		findNamedObject<wxPanel>(this, "ObjCompMainPanel")->Fit();
		Fit();
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
	_objective.description = _objDescriptionEntry->GetValue().ToStdString();

	// Save the difficulty settings
	_diffPanel->writeToObjective(_objective);

	// Set the initial state enum value from the combo box index
	_objective.state = static_cast<Objective::State>(
		wxutil::ChoiceHelper::GetSelectionId(_objStateCombo));

	// Determine which checkbox is toggled, then update the appropriate flag
	_objective.mandatory = _objMandatoryFlag->GetValue();
	_objective.visible = _objVisibleFlag->GetValue();
	_objective.ongoing = _objOngoingFlag->GetValue();
	_objective.irreversible = _objIrreversibleFlag->GetValue();

	// Enabling objectives
	_objective.enablingObjs = _enablingObjs->GetValue();

	// Success/failure logic
	_objective.logic.successLogic = _successLogic->GetValue();
	_objective.logic.failureLogic = _failureLogic->GetValue();

	// Completion/Failure script
	_objective.completionScript = _completionScript->GetValue();
	_objective.failureScript = _failureScript->GetValue();

	// Completion/Failure targets
	_objective.completionTarget = _completionTarget->GetValue();
	_objective.failureTarget = _failureTarget->GetValue();

	// Write the components
	checkWriteComponent();

	// Copy the working set over the ones in the objective
	_objective.components.swap(_components);
}

int ComponentsDialog::ShowModal()
{
	int returnCode = DialogBase::ShowModal();

	if (returnCode == wxID_OK)
	{
		save();
	}

	return returnCode;
}

void ComponentsDialog::_onCompToggleChanged(wxCommandEvent& ev)
{
	if (_updateMutex) return;

	// Update the Component working copy. The selected index must be valid, since the
	// edit panel is only sensitive if a component is selected
	int idx = getSelectedIndex();
	assert(idx >= 0);

	Component& comp = _components[idx];

	wxCheckBox* checkbox = dynamic_cast<wxCheckBox*>(ev.GetEventObject());

	if (checkbox == _stateFlag)
	{
		comp.setSatisfied(checkbox->GetValue());
	}
	else if (checkbox == _irreversibleFlag)
	{
		comp.setIrreversible(checkbox->GetValue());
	}
	else if (checkbox == _invertedFlag)
	{
		comp.setInverted(checkbox->GetValue());
	}
	else if (checkbox == _playerResponsibleFlag)
	{
		comp.setPlayerResponsible(checkbox->GetValue());
	}

	// Update the list store
	updateComponents();
}

void ComponentsDialog::handleSelectionChange()
{
	// Save the existing ComponentEditor contents if req'd
    checkWriteComponent();

	// Disconnect notification callback
	_componentChanged.disconnect();

	// Get the selection if valid
	wxDataViewItem item = _componentView->GetSelection();

	if (!item.IsOk())
    {
		// Disable the edit panel and remove the ComponentEditor
		_compEditorPanel->Enable(false);
		_editPanel->Enable(false);
		_componentEditor = objectives::ce::ComponentEditorPtr();
	}
	else
	{
		// Otherwise populate edit panel with the current component index
		wxutil::TreeModel::Row row(item, *_componentList);
		int index = row[_columns.index].getInteger();

		populateEditPanel(index);

		// Enable the edit panel
		_compEditorPanel->Enable(true);
		_editPanel->Enable(true);

		// Subscribe to the component
		Component& comp = _components[index];
		_componentChanged = comp.signal_Changed().connect(
			sigc::mem_fun(*this, &ComponentsDialog::_onComponentChanged));
	}
}

void ComponentsDialog::_onComponentChanged()
{
	wxDataViewItem item = _componentView->GetSelection();

	if (!item.IsOk()) return;

	updateComponents();
}

// Selection changed
void ComponentsDialog::_onSelectionChanged(wxDataViewEvent& ev)
{
    handleSelectionChange();
}

// Add a new component
void ComponentsDialog::_onAddComponent(wxCommandEvent& ev)
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
void ComponentsDialog::_onDeleteComponent(wxCommandEvent& ev)
{
	// Delete the selected component
	int idx = getSelectedIndex();

	if (idx != -1)
    {
        // Remove the selection first, so our selection-changed callback does not
        // attempt to writeToComponent() after the Component has already been deleted
		_componentView->UnselectAll();

		// UnselectAll doesn't seem to trigger a selection change
		handleSelectionChange();

        // Erase the actual component
		_components.erase(idx);

        // #5810: Re-index the remaining components (shift all higher ones by -1)
        auto highestKey = _components.rbegin();

        if (highestKey != _components.rend())
        {
            for (auto key = idx + 1; key <= highestKey->first; ++key)
            {
                // Attempt to move the component with the given key
                auto node = _components.extract(key);

                // If a component was unlinked from the map, move it downwards by one index
                if (!node.empty())
                {
                    node.key()--;
                    _components.insert(std::move(node));
                }
            }
        }
	}

	// Refresh the list
	populateComponents();
}

void ComponentsDialog::handleTypeChange()
{
	// Get the current selection
	int typeId = wxutil::ChoiceHelper::GetSelectionId(_typeCombo);

	// Update the Objective object. The selected index must be valid, since the
	// edit panel is only sensitive if a component is selected
	int idx = getSelectedIndex();
	assert(idx >= 0);

	Component& comp = _components[idx];

    // Store the newly-selected type in the Component
	comp.setType(ComponentType::getComponentType(typeId));

    // Change the ComponentEditor
    changeComponentEditor(comp);

	// Update the components list with the new display string
	wxutil::TreeModel::Row row(_componentView->GetSelection(), *_componentList);

	row[_columns.description] = comp.getString();
	row.SendItemChanged();

	_updateNeeded = true;
}

// Type combo changed
void ComponentsDialog::_onTypeChanged(wxCommandEvent& ev)
{
	handleTypeChange();
}

} // namespace objectives
