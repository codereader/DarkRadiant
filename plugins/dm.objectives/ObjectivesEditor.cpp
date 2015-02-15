#include "ObjectivesEditor.h"

#include "ObjectiveEntityFinder.h"
#include "RandomOrigin.h"
#include "TargetList.h"
#include "ComponentsDialog.h"
#include "MissionLogicDialog.h"
#include "ObjectiveConditionsDialog.h"
#include "util/ObjectivesException.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "imainframe.h"
#include "iregistry.h"
#include "ieclass.h"
#include "igame.h"
#include "ientity.h"
#include "iuimanager.h"

#include "wxutil/dialog/MessageBox.h"

#include <wx/button.h>
#include <wx/panel.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace objectives
{

// CONSTANTS
namespace
{
	const char* const DIALOG_TITLE = N_("Mission Objectives");

	const std::string RKEY_ROOT = "user/ui/objectivesEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	const std::string GKEY_OBJECTIVE_ENTS = "/objectivesEditor//objectivesEClass";
}

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor() :
	DialogBase(_(DIALOG_TITLE)),
	_objectiveEntityList(new wxutil::TreeModel(_objEntityColumns, true)),
	_objectiveList(new wxutil::TreeModel(_objectiveColumns, true))
{
	wxPanel* mainPanel = loadNamedPanel(this, "ObjDialogMainPanel");

    // Setup signals and tree views
    setupEntitiesPanel();
    setupObjectivesPanel();

    // Buttons not associated with a treeview panel
	wxButton* successLogicButton = findNamedObject<wxButton>(this, "ObjDialogSuccessLogicButton");
	successLogicButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onEditLogic), NULL, this);
	successLogicButton->Enable(false);
	
	wxButton* objCondButton = findNamedObject<wxButton>(this, "ObjDialogObjConditionsButton");
	objCondButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onEditObjConditions), NULL, this);
	objCondButton->Enable(false);

	findNamedObject<wxButton>(this, "ObjDialogCancelButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onCancel), NULL, this);

	findNamedObject<wxButton>(this, "ObjDialogOkButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onOK), NULL, this);

    _objectiveEClasses.clear();

    xml::NodeList nodes = GlobalGameManager().currentGame()->getLocalXPath(GKEY_OBJECTIVE_ENTS);

    for (const xml::Node& node : nodes)
    {
        _objectiveEClasses.push_back(node.getAttributeValue("name"));
    }

    mainPanel->Layout();
    mainPanel->Fit();
    Fit();
    CenterOnParent();

    // Remember the previous position or set up defaults
    _windowPosition.initialise(this, RKEY_WINDOW_STATE, 0.5f, 0.9f);
}

// Create the objects panel (for manipulating the target_addobjectives objects)
void ObjectivesEditor::setupEntitiesPanel()
{
	makeLabelBold(this, "ObjDialogEntityLabel");

	// Tree view listing the target_addobjectives entities
	wxPanel* entityPanel = findNamedObject<wxPanel>(this, "ObjDialogEntityPanel");

	// Entity Tree View
	_objectiveEntityView = wxutil::TreeView::CreateWithModel(entityPanel, _objectiveEntityList, wxDV_NO_HEADER);
	entityPanel->GetSizer()->Add(_objectiveEntityView, 1, wxEXPAND);

	_objectiveEntityView->AppendToggleColumn(_("Start"), _objEntityColumns.startActive.getColumnIndex(),
		wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	_objectiveEntityView->AppendTextColumn("", _objEntityColumns.displayName.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	_objectiveEntityView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ObjectivesEditor::_onEntitySelectionChanged), NULL, this);

	// Active-at-start column (checkbox)
	_objectiveEntityView->Connect(wxEVT_DATAVIEW_ITEM_EDITING_DONE, 
		wxDataViewEventHandler(ObjectivesEditor::_onStartActiveCellToggled), NULL, this);
	
    // Connect button signals
	findNamedObject<wxButton>(this, "ObjDialogAddEntityButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onAddEntity), NULL, this);

	wxButton* deleteButton = findNamedObject<wxButton>(this, "ObjDialogDeleteEntityButton");
	deleteButton->Enable(false); // disabled at start
	deleteButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onDeleteEntity), NULL, this);
}

// Create the main objective editing widgets
void ObjectivesEditor::setupObjectivesPanel()
{
	makeLabelBold(this, "ObjDialogObjectivesLabel");
	makeLabelBold(this, "ObjDialogLogicLabel");

	wxPanel* panel = findNamedObject<wxPanel>(this, "ObjDialogObjectivesPanel");

	// Entity Tree View
	_objectiveView = wxutil::TreeView::CreateWithModel(panel, _objectiveList);
	panel->GetSizer()->Add(_objectiveView, 1, wxEXPAND);

	// Key and value text columns
	_objectiveView->AppendTextColumn("#", _objectiveColumns.objNumber.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	_objectiveView->AppendTextColumn(_("Description"), _objectiveColumns.description.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	_objectiveView->AppendTextColumn(_("Diff."), _objectiveColumns.difficultyLevel.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	_objectiveView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(ObjectivesEditor::_onObjectiveSelectionChanged), NULL, this);
    
	wxButton* addButton = findNamedObject<wxButton>(this, "ObjDialogAddObjectiveButton");
	addButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onAddObjective), NULL, this);

	wxButton* editObjButton = findNamedObject<wxButton>(this, "ObjDialogEditObjectiveButton");
	editObjButton->Enable(false); // not enabled without selection
	editObjButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onEditObjective), NULL, this);

	wxButton* moveUpObjButton = findNamedObject<wxButton>(this, "ObjDialogMoveObjUpButton");
	moveUpObjButton->Enable(false); // not enabled without selection
	moveUpObjButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onMoveUpObjective), NULL, this);

	wxButton* moveDownObjButton = findNamedObject<wxButton>(this, "ObjDialogMoveObjDownButton");
	moveDownObjButton->Enable(false); // not enabled without selection
	moveDownObjButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onMoveDownObjective), NULL, this);

	wxButton* delObjButton = findNamedObject<wxButton>(this, "ObjDialogDeleteObjectiveButton");
	delObjButton->Enable(false); // not enabled without selection
	delObjButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onDeleteObjective), NULL, this);

	wxButton* clearObjButton = findNamedObject<wxButton>(this, "ObjDialogClearObjectiveButton");
	clearObjButton->Enable(false); // requires >0 objectives
	clearObjButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(ObjectivesEditor::_onClearObjectives), NULL, this);

	findNamedObject<wxPanel>(this, "ObjDialogObjectiveButtonPanel")->Enable(false);
}

void ObjectivesEditor::clear()
{
	// Clear internal data
	_worldSpawn = NULL;
	_entities.clear();
	_curEntity = _entities.end();

	// Clear the list boxes
	_objectiveEntityList->Clear();
	_objectiveList->Clear();

	_curObjective = wxDataViewItem();
	updateObjectiveButtonPanel();
}

// Populate widgets with map data
void ObjectivesEditor::populateWidgets()
{
	// Clear internal data first
	clear();

	// Use an ObjectiveEntityFinder to walk the map and add any objective
	// entities to the liststore and entity map
	ObjectiveEntityFinder finder(
        _objectiveEntityList, _objEntityColumns, _entities, _objectiveEClasses
    );
	GlobalSceneGraph().root()->traverse(finder);

	// Set the worldspawn entity and populate the active-at-start column
	_worldSpawn = finder.getWorldSpawn();
	if (_worldSpawn != NULL)
	{
		populateActiveAtStart();
	}
}

// Populate the active-at-start column.
void ObjectivesEditor::populateActiveAtStart()
{
	// Construct the list of entities targeted by the worldspawn
	TargetList targets(_worldSpawn);

	// Iterate through each row in the entity list. For each Entity*, get its
	// name and check if the worldspawn entity has a "target" key for this
	// entity name. This indicates that the objective entity will be active at
	// game start.
	_objectiveEntityList->ForeachNode([&] (wxutil::TreeModel::Row& row)
	{
		std::string name = row[_objEntityColumns.entityName];

		ObjectiveEntityPtr obj = _entities[name];

		// Test if the worldspawn is targeting this entity by passing the
		// target list to the objective entity.
		if (obj->isOnTargetList(targets))
		{
			row[_objEntityColumns.startActive] = true;
		}
	});
}

int ObjectivesEditor::ShowModal()
{
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();

	int returnValue = DialogBase::ShowModal();

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	// Clear all data before hiding
	clear();

	return returnValue;
}

// Static method to display dialog
void ObjectivesEditor::DisplayDialog(const cmd::ArgumentList& args)
{
	// Create a new dialog instance
	ObjectivesEditor* _instance = new ObjectivesEditor;

	try
	{
		// Show the instance
		_instance->ShowModal();
	}
	catch (ObjectivesException& e)
	{
		wxutil::Messagebox::ShowError(
			std::string(_("Exception occurred: ")) + e.what()
		);
	}

	_instance->Destroy();
}

// Refresh the objectives list from the ObjectiveEntity
void ObjectivesEditor::refreshObjectivesList()
{
	_curObjective = wxDataViewItem();
	updateObjectiveButtonPanel();

	// Clear and refresh the objective list
	_objectiveList->Clear();
	_curEntity->second->populateListStore(*_objectiveList, _objectiveColumns);

	// If there is at least one objective, make the Clear button available
    wxButton* clearObjButton = findNamedObject<wxButton>(this, "ObjDialogClearObjectiveButton");
	clearObjButton->Enable(!_curEntity->second->isEmpty());
}

// Get the currently selected objective
Objective& ObjectivesEditor::getCurrentObjective()
{
	// Get the objective index from the list
	wxutil::TreeModel::Row row(_curObjective, *_objectiveList);
	int objNum = row[_objectiveColumns.objNumber].getInteger();

	// Pass the index to the ObjectiveEntity to get an actual Objective
	return _curEntity->second->getObjective(objNum);
}

void ObjectivesEditor::_onCancel(wxCommandEvent& ev)
{
	// Close the window without saving
	EndModal(wxID_CANCEL);
}

// OK button
void ObjectivesEditor::_onOK(wxCommandEvent& ev)
{
	// Write all ObjectiveEntity data to the underlying entities
	for (ObjectiveEntityMap::iterator i = _entities.begin();
		 i != _entities.end();
		 ++i)
	{
		i->second->writeToEntity();
	}

	// Close the window
	EndModal(wxID_OK);
}

void ObjectivesEditor::selectObjectiveByIndex(int index)
{
	if (index == -1) return;

	// Select the new objective
	wxDataViewItem newObjLoc = _objectiveList->FindInteger(index, 
			_objectiveColumns.objNumber);

	_objectiveView->Select(newObjLoc);
	_curObjective = newObjLoc;
	updateObjectiveButtonPanel();
}

// Callback for "start active" cell toggle in entities list
void ObjectivesEditor::_onStartActiveCellToggled(wxDataViewEvent& ev)
{
}

// Callback for objective entity selection changed in list box
void ObjectivesEditor::_onEntitySelectionChanged(wxDataViewEvent& ev)
{
	// Clear the objectives list
	_objectiveList->Clear();
	
	updateEditorButtonPanel();
}

void ObjectivesEditor::updateObjectiveButtonPanel()
{
	wxButton* editObjButton = findNamedObject<wxButton>(this, "ObjDialogEditObjectiveButton");
	wxButton* delObjButton = findNamedObject<wxButton>(this, "ObjDialogDeleteObjectiveButton");
	wxButton* moveUpButton = findNamedObject<wxButton>(this, "ObjDialogMoveObjUpButton");
	wxButton* moveDownButton = findNamedObject<wxButton>(this, "ObjDialogMoveObjDownButton");

	if (_curObjective.IsOk())
    {
        // Enable the edit and delete buttons
        editObjButton->Enable(true);
        delObjButton->Enable(true);

        // Check if this is the first command in the list, get the ID of the
        // selected item
		wxutil::TreeModel::Row row(_curObjective, *_objectiveList);
		int index = row[_objectiveColumns.objNumber].getInteger();

        int highestIndex = _curEntity->second->getHighestObjIndex();
        int lowestIndex = _curEntity->second->getLowestObjIndex();

        bool hasNext = (highestIndex != -1 && highestIndex > index);
        bool hasPrev = (lowestIndex != -1 && lowestIndex < index);

        moveUpButton->Enable(hasPrev);
        moveDownButton->Enable(hasNext);
	}
	else
    {
		// Disable the edit, delete and move buttons
		editObjButton->Enable(false);
		delObjButton->Enable(false);
		moveUpButton->Enable(false);
        moveDownButton->Enable(false);
	}
}

// Callback for current objective selection changed
void ObjectivesEditor::_onObjectiveSelectionChanged(wxDataViewEvent& ev)
{
	// Get the selection
	_curObjective = ev.GetItem();

	updateObjectiveButtonPanel();
}

void ObjectivesEditor::updateEditorButtonPanel()
{
	wxButton* delEntityButton = findNamedObject<wxButton>(this, "ObjDialogDeleteEntityButton");
	wxPanel* objButtonPanel = findNamedObject<wxPanel>(this, "ObjDialogObjectiveButtonPanel");

	wxButton* successLogicButton = findNamedObject<wxButton>(this, "ObjDialogSuccessLogicButton");
	wxButton* objCondButton = findNamedObject<wxButton>(this, "ObjDialogObjConditionsButton");

	// Get the selection
	wxDataViewItem item = _objectiveEntityView->GetSelection();

	if (item.IsOk()) 
    {
		// Get name of the entity and find the corresponding ObjectiveEntity in
		// the map
		wxutil::TreeModel::Row row(item, *_objectiveEntityList);
		std::string name = row[_objEntityColumns.entityName];

		// Save the current selection and refresh the objectives list
		_curEntity = _entities.find(name);
		refreshObjectivesList();

		// Enable the delete button and objectives panel
		delEntityButton->Enable(true);
        objButtonPanel->Enable(true);

        // Enable buttons
        successLogicButton->Enable(true);
        objCondButton->Enable(true);
	}
	else
    {
		// No selection, disable the delete button and clear the objective
		// panel
		delEntityButton->Enable(false);
		objButtonPanel->Enable(false);

        // Disable mission logic button
        successLogicButton->Enable(false);
        objCondButton->Enable(false);
	}
}

// Add a new objectives entity button
void ObjectivesEditor::_onAddEntity(wxCommandEvent& ev)
{
	if (_objectiveEClasses.empty())
	{
		// Objective entityclass(es) not defined
        wxutil::Messagebox::ShowError(
            _("Unable to create Objective Entity: classes not defined in registry.")
        );
		return;
	}

	const std::string& objEClass = _objectiveEClasses.front();

	// Obtain the entity class object
	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(objEClass);
		
    if (eclass) 
    {
        // Construct a Node of this entity type
        IEntityNodePtr node(GlobalEntityCreator().createEntity(eclass));

        // Create a random offset
        node->getEntity().setKeyValue("origin", RandomOrigin::generate(128));

        // Insert the node into the scene graph
        assert(GlobalSceneGraph().root());
        GlobalSceneGraph().root()->addChildNode(node);

        // Refresh the widgets
        populateWidgets();
    }
    else
    {
        // Objective entityclass was not found
        wxutil::Messagebox::ShowError(
			(boost::format(_("Unable to create Objective Entity: class '%s' not found.")) % objEClass).str()
        );
    }
}

// Delete entity button
void ObjectivesEditor::_onDeleteEntity(wxCommandEvent& ev)
{
	// Get the selection
    wxDataViewItem item = _objectiveEntityView->GetSelection();
	
	if (item.IsOk()) 
	{
		// Get the name of the selected entity
		wxutil::TreeModel::Row row(item, *_objectiveEntityList);
		std::string name = row[_objEntityColumns.entityName];

		// Instruct the ObjectiveEntity to delete its world node, and then
		// remove it from the map
		_entities[name]->deleteWorldNode();
		_entities.erase(name);

		// Update the widgets to remove the selection from the list
		populateWidgets();

		updateEditorButtonPanel();
	}
}

// Add a new objective
void ObjectivesEditor::_onAddObjective(wxCommandEvent& ev)
{
	// Add a new objective to the ObjectiveEntity and refresh the list store
	_curEntity->second->addObjective();
	refreshObjectivesList();
}

// Edit an existing objective
void ObjectivesEditor::_onEditObjective(wxCommandEvent& ev)
{
	// Display the ComponentsDialog
	ComponentsDialog* compDialog = new ComponentsDialog(this, getCurrentObjective());
	
	compDialog->ShowModal(); // show and block
	compDialog->Destroy();

	// Repopulate the objective list
	refreshObjectivesList();
}

void ObjectivesEditor::_onMoveUpObjective(wxCommandEvent& ev)
{
	// get the current index
	wxutil::TreeModel::Row row(_curObjective, *_objectiveList);
	int index = row[_objectiveColumns.objNumber].getInteger();

	// Pass the call to the general method
	int newIndex = _curEntity->second->moveObjective(index, -1);

	refreshObjectivesList();
	selectObjectiveByIndex(newIndex);
}

void ObjectivesEditor::_onMoveDownObjective(wxCommandEvent& ev)
{
	// get the current index
	wxutil::TreeModel::Row row(_curObjective, *_objectiveList);
	int index = row[_objectiveColumns.objNumber].getInteger();

	// Pass the call to the general method
	int newIndex = _curEntity->second->moveObjective(index, +1);

	refreshObjectivesList();
	selectObjectiveByIndex(newIndex);
}

// Delete an objective
void ObjectivesEditor::_onDeleteObjective(wxCommandEvent& ev)
{
	// Get the index of the current objective
	wxutil::TreeModel::Row row(_curObjective, *_objectiveList);
	int index = row[_objectiveColumns.objNumber].getInteger();

	// Tell the ObjectiveEntity to delete this objective
	_curEntity->second->deleteObjective(index);

	// Repopulate the objective list
	refreshObjectivesList();
}

// Clear the objectives
void ObjectivesEditor::_onClearObjectives(wxCommandEvent& ev)
{
	// Clear the entity and refresh the list
	_curEntity->second->clearObjectives();
	refreshObjectivesList();
}

void ObjectivesEditor::_onEditLogic(wxCommandEvent& ev)
{
	MissionLogicDialog* dialog = new MissionLogicDialog(this, *_curEntity->second);
	
	dialog->ShowModal();
	dialog->Destroy();

	refreshObjectivesList();
}

void ObjectivesEditor::_onEditObjConditions(wxCommandEvent& ev)
{
	ObjectiveConditionsDialog* dialog = new ObjectiveConditionsDialog(this, *_curEntity->second);
	
	dialog->ShowModal();
	dialog->Destroy();

	refreshObjectivesList();
}

} // namespace objectives
