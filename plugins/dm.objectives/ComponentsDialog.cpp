#include "ComponentsDialog.h"
#include "Objective.h"
#include "ce/ComponentEditorFactory.h"
#include "util/TwoColumnTextCombo.h"

#include "itextstream.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "string/string.h"

#include "i18n.h"
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/separator.h>
#include <gtkmm/table.h>
#include <gtkmm/frame.h>
#include <gtkmm/entry.h>
#include <gtkmm/comboboxtext.h>

namespace objectives
{

/* CONSTANTS */

namespace {

	const char* const DIALOG_TITLE = N_("Edit Objective");

	inline std::string makeBold(const std::string& input)
	{
		return "<b>" + input + "</b>";
	}

} // namespace

// Main constructor
ComponentsDialog::ComponentsDialog(const Glib::RefPtr<Gtk::Window>& parent, Objective& objective) :
	gtkutil::BlockingTransientWindow(_(DIALOG_TITLE), parent),
	_objective(objective),
	_componentList(Gtk::ListStore::create(_columns)),
	_diffPanel(Gtk::manage(new DifficultyPanel)),
	_components(objective.components), // copy the components to our local working set
	_updateMutex(false),
	_timer(500, _onIntervalReached, this)
{
	// Dialog contains list view, edit panel and buttons
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 12));
	vbx->pack_start(createObjectiveEditPanel(), false, false, 0);

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Components")))), false, false, 0);

	Gtk::VBox* compvbox = Gtk::manage(new Gtk::VBox(false, 6));
	compvbox->pack_start(createListView(), true, true, 0);
	compvbox->pack_start(createEditPanel(), false, false, 0);
	compvbox->pack_start(createComponentEditorPanel(), true, true, 0);

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*compvbox, 12, 1.0f)), true, true, 0);

	vbx->pack_start(*Gtk::manage(new Gtk::HSeparator), false, false, 0);
	vbx->pack_end(createButtons(), false, false, 0);

	// Populate the list of components
	populateObjectiveEditPanel();
	populateComponents();

	// Add contents to main window
	set_border_width(12);
	add(*vbx);
}

ComponentsDialog::~ComponentsDialog()
{
	_timer.disable();
}

// Create the panel for editing the currently-selected objective
Gtk::Widget& ComponentsDialog::createObjectiveEditPanel()
{
	// Table for entry boxes
	Gtk::Table* table = Gtk::manage(new Gtk::Table(8, 2, false));
	table->set_row_spacings(6);
	table->set_col_spacings(12);

	int row = 0;

	// Objective description
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Description")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);

	_objDescriptionEntry = Gtk::manage(new Gtk::Entry);
	table->attach(*_objDescriptionEntry, 1, 2, row, row+1);

	row++;

	// Difficulty Selection
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Difficulty")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_diffPanel, 1, 2, row, row+1);

	row++;

	// State selection
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Initial state")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);

	_objStateCombo = Gtk::manage(new Gtk::ComboBoxText);
	table->attach(*_objStateCombo, 1, 2, row, row+1);

	// Populate the list of states. This must be done in order to match the
	// values in the enum, since the index will be used when writing to entity
	_objStateCombo->append_text(Objective::getStateText(Objective::INCOMPLETE));
	_objStateCombo->append_text(Objective::getStateText(Objective::COMPLETE));
	_objStateCombo->append_text(Objective::getStateText(Objective::INVALID));
	_objStateCombo->append_text(Objective::getStateText(Objective::FAILED));

	row++;

	// Options checkboxes.
	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Flags")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(createObjectiveFlagsTable(), 1, 2, row, row+1);

	row++;

	// Enabling objectives
	_enablingObjs = Gtk::manage(new Gtk::Entry);

	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Enabling Objectives")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_enablingObjs, 1, 2, row, row+1);

	row++;

	// Logic
	Gtk::HBox* logicHBox = Gtk::manage(new Gtk::HBox(false, 6));

	// Success Logic
	_successLogic = Gtk::manage(new Gtk::Entry);

	// Failure Logic
	_failureLogic = Gtk::manage(new Gtk::Entry);

	logicHBox->pack_start(*_successLogic, true, true, 0);
	logicHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Failure Logic")))), false, false, 0);
	logicHBox->pack_start(*_failureLogic, true, true, 0);

	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Sucess Logic")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*logicHBox, 1, 2, row, row+1);

	row++;

	// Completion/failure scripts
	Gtk::HBox* scriptHBox = Gtk::manage(new Gtk::HBox(false, 6));

	// Completion Script
	_completionScript = Gtk::manage(new Gtk::Entry);

	// Failure Script
	_failureScript = Gtk::manage(new Gtk::Entry);

	scriptHBox->pack_start(*_completionScript, true, true, 0);
	scriptHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Failure Script")))), false, false, 0);
	scriptHBox->pack_start(*_failureScript, true, true, 0);

	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Completion Script")))),
				  0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*scriptHBox, 1, 2, row, row+1);

	row++;

	// Completion/failure targets
	Gtk::HBox* targetHBox = Gtk::manage(new Gtk::HBox(false, 6));

	// Completion Target
	_completionTarget = Gtk::manage(new Gtk::Entry);

	// Failure Target
	_failureTarget = Gtk::manage(new Gtk::Entry);

	targetHBox->pack_start(*_completionTarget, true, true, 0);
	targetHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Failure Target")))), false, false, 0);
	targetHBox->pack_start(*_failureTarget, true, true, 0);

	table->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Completion Target")))),
					 0, 1, row, row+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*targetHBox, 1, 2, row, row+1);

	// Pack items into a vbox and return
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 6));
	vbx->pack_start(*table, false, false, 0);
	return *vbx;
}

// Create table of flag checkboxes
Gtk::Widget& ComponentsDialog::createObjectiveFlagsTable()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 12));

	_objMandatoryFlag = Gtk::manage(new Gtk::CheckButton(_("Mandatory")));
	_objIrreversibleFlag = Gtk::manage(new Gtk::CheckButton(_("Irreversible")));
	_objOngoingFlag = Gtk::manage(new Gtk::CheckButton(_("Ongoing")));
	_objVisibleFlag = Gtk::manage(new Gtk::CheckButton(_("Visible")));

	hbx->pack_start(*_objMandatoryFlag, false, false, 0);
	hbx->pack_start(*_objOngoingFlag, false, false, 0);
	hbx->pack_start(*_objIrreversibleFlag, false, false, 0);
	hbx->pack_start(*_objVisibleFlag, false, false, 0);

	return *hbx;
}

// Create list view
Gtk::Widget& ComponentsDialog::createListView()
{
	// Create tree view and connect selection changed callback
	_componentView = Gtk::manage(new Gtk::TreeView(_componentList));

	_componentView->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ComponentsDialog::_onSelectionChanged));

	// Number column
	_componentView->append_column(*Gtk::manage(new gtkutil::TextColumn("#", _columns.index, false)));
	_componentView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Type"), _columns.description, false)));

	// Create Add and Delete buttons for components
	Gtk::Button* addButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	Gtk::Button* delButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));

	addButton->signal_clicked().connect(sigc::mem_fun(*this, &ComponentsDialog::_onAddComponent));
	delButton->signal_clicked().connect(sigc::mem_fun(*this, &ComponentsDialog::_onDeleteComponent));

	Gtk::VBox* buttonsBox = Gtk::manage(new Gtk::VBox(false, 6));
	buttonsBox->pack_start(*addButton, true, true, 0);
	buttonsBox->pack_start(*delButton, true, true, 0);

	// Put the buttons box next to the list view
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));
	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_componentView)), true, true, 0);
	hbx->pack_start(*buttonsBox, false, false, 0);

	return *hbx;
}

// Create edit panel
Gtk::Widget& ComponentsDialog::createEditPanel()
{
	// Table
	_editPanel = Gtk::manage(new Gtk::Table(2, 2, false));
	_editPanel->set_row_spacings(12);
	_editPanel->set_col_spacings(12);
	_editPanel->set_sensitive(false); // disabled until selection

	// Component type dropdown
	_typeCombo = Gtk::manage(new util::TwoColumnTextCombo);
	_typeCombo->signal_changed().connect(sigc::mem_fun(*this, &ComponentsDialog::_onTypeChanged));

	// Pack dropdown into table
	_editPanel->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Type")))),
					   0, 1, 0, 1, Gtk::FILL, Gtk::FILL, 0, 0);

	_editPanel->attach(*_typeCombo, 1, 2, 0, 1);

	// Populate the combo box. The set is in ID order.
	for (ComponentTypeSet::const_iterator i = ComponentType::SET_ALL().begin();
		 i != ComponentType::SET_ALL().end();
		 ++i)
	{
		Glib::RefPtr<Gtk::ListStore> ls =
			Glib::RefPtr<Gtk::ListStore>::cast_static(_typeCombo->get_model());

		Gtk::TreeModel::Row row = *ls->append();

		row.set_value(0, i->getDisplayName());
		row.set_value(1, i->getName());
	}

	// Flags hbox
	_stateFlag = Gtk::manage(new Gtk::CheckButton(_("Satisfied at start")));
	_irreversibleFlag = Gtk::manage(new Gtk::CheckButton(_("Irreversible")));
	_invertedFlag = Gtk::manage(new Gtk::CheckButton(_("Boolean NOT")));
	_playerResponsibleFlag = Gtk::manage(new Gtk::CheckButton(_("Player responsible")));

	_stateFlag->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &ComponentsDialog::_onCompToggleChanged), _stateFlag));
	_irreversibleFlag->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &ComponentsDialog::_onCompToggleChanged),_irreversibleFlag));
	_invertedFlag->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &ComponentsDialog::_onCompToggleChanged),_invertedFlag));
	_playerResponsibleFlag->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &ComponentsDialog::_onCompToggleChanged), _playerResponsibleFlag));

	Gtk::HBox* flagsBox = Gtk::manage(new Gtk::HBox(false, 12));
	flagsBox->pack_start(*_stateFlag, false, false, 0);
	flagsBox->pack_start(*_irreversibleFlag, false, false, 0);
	flagsBox->pack_start(*_invertedFlag, false, false, 0);
	flagsBox->pack_start(*_playerResponsibleFlag, false, false, 0);

	_editPanel->attach(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Flags")))),
					 0, 1, 1, 2, Gtk::FILL, Gtk::FILL, 0, 0);
	_editPanel->attach(*flagsBox, 1, 2, 1, 2, Gtk::FILL, Gtk::FILL, 0, 0);

	return *_editPanel;
}

// ComponentEditor panel
Gtk::Widget& ComponentsDialog::createComponentEditorPanel()
{
    // Invisible frame to contain the ComponentEditor
	_compEditorPanel = Gtk::manage(new Gtk::Frame);
	_compEditorPanel->set_shadow_type(Gtk::SHADOW_NONE);
    _compEditorPanel->set_border_width(6);

    // Visible frame
	Gtk::Frame* borderFrame = Gtk::manage(new Gtk::Frame);
    borderFrame->add(*_compEditorPanel);

	return *borderFrame;
}

// Create buttons
Gtk::Widget& ComponentsDialog::createButtons()
{
	// Create a new homogeneous hbox
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ComponentsDialog::_onOK));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ComponentsDialog::_onCancel));

	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
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
