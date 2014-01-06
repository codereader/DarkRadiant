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

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog/MessageBox.h"

#include <gtkmm/treeview.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gtkmm/separator.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace objectives
{

// CONSTANTS
namespace {

	const char* const DIALOG_TITLE = N_("Mission Objectives");

	const std::string RKEY_ROOT = "user/ui/objectivesEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	const std::string GKEY_OBJECTIVE_ENTS = "/objectivesEditor//objectivesEClass";
}

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor() :
	gtkutil::BlockingTransientWindow(
        _(DIALOG_TITLE), GlobalMainFrame().getTopLevelWindow()
    ),
    gtkutil::GladeWidgetHolder("ObjectivesEditor.glade"),
	_objectiveEntityList(Gtk::ListStore::create(_objEntityColumns)),
	_objectiveList(Gtk::ListStore::create(_objectiveColumns))
{
    // Window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
    
    // Add vbox to dialog
    add(*gladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != NULL);

    // Setup signals and tree views
    setupEntitiesPanel();
    setupObjectivesPanel();

    // Buttons not associated with a treeview panel
    Gtk::Button* logicButton = gladeWidget<Gtk::Button>(
        "editSuccessLogicButton"
    );
    logicButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onEditLogic)
    );
	Gtk::Button* conditionsButton = gladeWidget<Gtk::Button>(
        "editObjectiveConditionsButton"
    );
    conditionsButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onEditObjConditions)
    );

    gladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onCancel)
    );
	gladeWidget<Gtk::Button>("okButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onOK)
    );

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    _objectiveEClasses.clear();

    xml::NodeList nodes = GlobalGameManager().currentGame()->getLocalXPath(GKEY_OBJECTIVE_ENTS);

    for (xml::NodeList::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        _objectiveEClasses.push_back(i->getAttributeValue("name"));
    }
}

// Create the objects panel (for manipulating the target_addobjectives objects)
void ObjectivesEditor::setupEntitiesPanel()
{
	// Tree view listing the target_addobjectives entities
    Gtk::TreeView* entityList = gladeWidget<Gtk::TreeView>(
        "entitiesTreeView"
    );
    entityList->set_model(_objectiveEntityList);
	entityList->set_headers_visible(false);

	entityList->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ObjectivesEditor::_onEntitySelectionChanged)
    );
	
	// Active-at-start column (checkbox)
	Gtk::CellRendererToggle* startToggle = Gtk::manage(new Gtk::CellRendererToggle);
	startToggle->signal_toggled().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onStartActiveCellToggled)
    );
	
	Gtk::TreeViewColumn* startCol = Gtk::manage(new Gtk::TreeViewColumn(_("Start")));
	startCol->add_attribute(startToggle->property_active(), _objEntityColumns.startActive);
	
	entityList->append_column(*startCol);
	
	// Name column
	entityList->append_column(*Gtk::manage(new gtkutil::TextColumn("", _objEntityColumns.displayName)));
	
    // Connect button signals
    Gtk::Button* addButton = gladeWidget<Gtk::Button>("createEntityButton");
	addButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onAddEntity)
    );

    Gtk::Button* delButton = gladeWidget<Gtk::Button>("deleteEntityButton");
	delButton->set_sensitive(false); // disabled at start
	delButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onDeleteEntity)
    );
}

// Create the main objective editing widgets
void ObjectivesEditor::setupObjectivesPanel()
{
    // Tree view
    Gtk::TreeView* objList = gladeWidget<Gtk::TreeView>(
        "objectivesTreeView"
    );
    objList->set_model(_objectiveList);
    objList->set_headers_visible(true);

    objList->get_selection()->signal_changed().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onObjectiveSelectionChanged)
    );
    
    // Key and value text columns
    objList->append_column(*Gtk::manage(
        new gtkutil::TextColumn("#", _objectiveColumns.objNumber, false)));
    objList->append_column(*Gtk::manage(
        new gtkutil::TextColumn(_("Description"), _objectiveColumns.description, false)));
    objList->append_column(*Gtk::manage(
        new gtkutil::TextColumn(_("Diff."), _objectiveColumns.difficultyLevel, false)));
    
    Gtk::Button* addButton = gladeWidget<Gtk::Button>("addObjButton");
    addButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onAddObjective)
    );

    Gtk::Button* editObjButton = gladeWidget<Gtk::Button>(
        "editObjButton"
    );
    editObjButton->set_sensitive(false); // not enabled without selection 
    editObjButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onEditObjective)
    );

    Gtk::Button* moveUpObjButton = gladeWidget<Gtk::Button>(
        "objMoveUpButton"
    );
    moveUpObjButton->set_sensitive(false); // not enabled without selection 
    moveUpObjButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onMoveUpObjective)
    );

    Gtk::Button* moveDownObjButton = gladeWidget<Gtk::Button>(
        "objMoveDownButton"
    );
    moveDownObjButton->set_sensitive(false); // not enabled without selection 
    moveDownObjButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onMoveDownObjective)
    );

    Gtk::Button* delObjButton = gladeWidget<Gtk::Button>(
        "delObjButton"
    );
    delObjButton->set_sensitive(false); // not enabled without selection 
    delObjButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onDeleteObjective)
    );
    
    Gtk::Button* clearObjButton = gladeWidget<Gtk::Button>(
        "clearObjectivesButton"
    );
    clearObjButton->set_sensitive(false); // requires >0 objectives
    clearObjButton->signal_clicked().connect(
        sigc::mem_fun(*this, &ObjectivesEditor::_onClearObjectives)
    );
}

void ObjectivesEditor::clear()
{
	// Clear internal data
	_worldSpawn = NULL;
	_entities.clear();
	_curEntity = _entities.end();

	// Clear the list boxes
	_objectiveEntityList->clear();
	_objectiveList->clear();
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
	Gtk::TreeModel::Children rows = _objectiveEntityList->children();

	for (Gtk::TreeModel::Children::iterator i = rows.begin(); i != rows.end(); ++i)
	{
		std::string name = Glib::ustring((*i)[_objEntityColumns.entityName]);

		ObjectiveEntityPtr obj = _entities[name];

		// Test if the worldspawn is targeting this entity by passing the
		// target list to the objective entity.
		if (obj->isOnTargetList(targets))
		{
			(*i)[_objEntityColumns.startActive] = true;
		}
	}
}

void ObjectivesEditor::_preHide()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	// Clear all data before hiding
	clear();
}

void ObjectivesEditor::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();

	populateWidgets();
}

// Static method to display dialog
void ObjectivesEditor::displayDialog(const cmd::ArgumentList& args)
{
	// Create a new dialog instance
	ObjectivesEditor _instance;

	try {
		// Show the instance
		_instance.show();
	}
	catch (ObjectivesException& e)
	{
		gtkutil::MessageBox::ShowError(
			std::string(_("Exception occurred: ")) + e.what(),
			GlobalMainFrame().getTopLevelWindow()
		);

		if (_instance.is_visible())
		{
			_instance.destroy();
		}
	}
}

// Refresh the objectives list from the ObjectiveEntity
void ObjectivesEditor::refreshObjectivesList()
{
	// Clear and refresh the objective list
	_objectiveList->clear();
	_curEntity->second->populateListStore(_objectiveList, _objectiveColumns);

	// If there is at least one objective, make the Clear button available
    Gtk::Button* clearObjButton = gladeWidget<Gtk::Button>(
        "clearObjectivesButton"
    );
	if (_curEntity->second->isEmpty())
	{
		clearObjButton->set_sensitive(false);
	}
	else
	{
		clearObjButton->set_sensitive(true);
	}
}

// Get the currently selected objective
Objective& ObjectivesEditor::getCurrentObjective()
{
	// Get the objective index from the list
	int objNum = (*_curObjective)[_objectiveColumns.objNumber];

	// Pass the index to the ObjectiveEntity to get an actual Objective
	return _curEntity->second->getObjective(objNum);
}

void ObjectivesEditor::_onCancel()
{
	// Close the window without saving
	destroy();
}

// OK button
void ObjectivesEditor::_onOK()
{
	// Write all ObjectiveEntity data to the underlying entities
	for (ObjectiveEntityMap::iterator i = _entities.begin();
		 i != _entities.end();
		 ++i)
	{
		i->second->writeToEntity();
	}

	// Close the window
	destroy();
}

// Callback for "start active" cell toggle in entities list
void ObjectivesEditor::_onStartActiveCellToggled(const Glib::ustring& path)
{
	// Get the relevant row
	Gtk::TreeModel::iterator iter = _objectiveEntityList->get_iter(path);

	// Toggle the state of the column
	bool current = (*iter)[_objEntityColumns.startActive];
	(*iter)[_objEntityColumns.startActive] = !current;
}

// Callback for objective entity selection changed in list box
void ObjectivesEditor::_onEntitySelectionChanged()
{
	// Clear the objectives list
	_objectiveList->clear();
	
    Gtk::Button* delEntityButton = gladeWidget<Gtk::Button>(
        "deleteEntityButton"
    );
    Gtk::Widget* objButtonPanel = gladeWidget<Gtk::Widget>(
        "objButtonPanel"
    );

	// Get the selection
    Gtk::TreeView* entityList = gladeWidget<Gtk::TreeView>(
        "entitiesTreeView"
    );
	Gtk::TreeModel::iterator iter = entityList->get_selection()->get_selected();
	if (iter) 
    {
		// Get name of the entity and find the corresponding ObjectiveEntity in
		// the map
		std::string name = Glib::ustring((*iter)[_objEntityColumns.entityName]);

		// Save the current selection and refresh the objectives list
		_curEntity = _entities.find(name);
		refreshObjectivesList();

		// Enable the delete button and objectives panel
		delEntityButton->set_sensitive(true);
        objButtonPanel->set_sensitive(true);

        // Enable mission logic button
        gladeWidget<Gtk::Widget>(
            "editSuccessLogicButton"
        )->set_sensitive(true);

		// Enable obj condition button
        gladeWidget<Gtk::Widget>(
            "editObjectiveConditionsButton"
        )->set_sensitive(true);
	}
	else
    {
		// No selection, disable the delete button and clear the objective
		// panel
		delEntityButton->set_sensitive(false);
		objButtonPanel->set_sensitive(false);

        // Disable mission logic button
        gladeWidget<Gtk::Widget>(
            "editSuccessLogicButton"
        )->set_sensitive(false);

		// Disable obj condition button
        gladeWidget<Gtk::Widget>(
            "editObjectiveConditionsButton"
        )->set_sensitive(false);
	}
}

// Callback for current objective selection changed
void ObjectivesEditor::_onObjectiveSelectionChanged()
{
	// Get the selection
	_curObjective = gladeWidget<Gtk::TreeView>("objectivesTreeView")
                    ->get_selection()->get_selected();

	if (_curObjective)
    {
        // Enable the edit and delete buttons
        gladeWidget<Gtk::Widget>("editObjButton")->set_sensitive(true);
        gladeWidget<Gtk::Widget>("delObjButton")->set_sensitive(true);

        // Check if this is the first command in the list, get the ID of the
        // selected item
        int index = (*_curObjective)[_objectiveColumns.objNumber];

        int highestIndex = _curEntity->second->getHighestObjIndex();
        int lowestIndex = _curEntity->second->getLowestObjIndex();

        bool hasNext = (highestIndex != -1 && highestIndex > index);
        bool hasPrev = (lowestIndex != -1 && lowestIndex < index);

        gladeWidget<Gtk::Widget>("objMoveUpButton")->set_sensitive(hasPrev);
        gladeWidget<Gtk::Widget>("objMoveDownButton")->set_sensitive(hasNext);
	}
	else
    {
		// Disable the edit, delete and move buttons
		gladeWidget<Gtk::Widget>("editObjButton")->set_sensitive(false);
		gladeWidget<Gtk::Widget>("delObjButton")->set_sensitive(false);
		gladeWidget<Gtk::Widget>("objMoveUpButton")->set_sensitive(false);
        gladeWidget<Gtk::Widget>("objMoveDownButton")->set_sensitive(false);
	}
}

// Add a new objectives entity button
void ObjectivesEditor::_onAddEntity()
{
	if (_objectiveEClasses.empty())
	{
		// Objective entityclass(es) not defined
        gtkutil::MessageBox::ShowError(
            _("Unable to create Objective Entity: classes not defined in registry."),
            GlobalMainFrame().getTopLevelWindow()
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
        gtkutil::MessageBox::ShowError(
			(boost::format(_("Unable to create Objective Entity: class '%s' not found.")) % objEClass).str(),
            GlobalMainFrame().getTopLevelWindow()
        );
    }
}

// Delete entity button
void ObjectivesEditor::_onDeleteEntity()
{
	// Get the selection
    Gtk::TreeView* entityList = gladeWidget<Gtk::TreeView>(
        "entitiesTreeView"
    );
    Gtk::TreeModel::iterator iter = entityList->get_selection()->get_selected();
	
	if (iter) 
	{
		// Get the name of the selected entity
		std::string name = Glib::ustring((*iter)[_objEntityColumns.entityName]);

		// Instruct the ObjectiveEntity to delete its world node, and then
		// remove it from the map
		_entities[name]->deleteWorldNode();
		_entities.erase(name);

		// Update the widgets to remove the selection from the list
		populateWidgets();
	}
}

// Add a new objective
void ObjectivesEditor::_onAddObjective()
{
	// Add a new objective to the ObjectiveEntity and refresh the list store
	_curEntity->second->addObjective();
	refreshObjectivesList();
}

// Edit an existing objective
void ObjectivesEditor::_onEditObjective()
{
	// Display the ComponentsDialog
	ComponentsDialog compDialog(getRefPtr(), getCurrentObjective());
	compDialog.show(); // show and block

	// Repopulate the objective list
	refreshObjectivesList();
}

void ObjectivesEditor::_onMoveUpObjective()
{
	// get the current index
	int index = (*_curObjective)[_objectiveColumns.objNumber];

	// Pass the call to the general method
	_curEntity->second->moveObjective(index, -1);

	refreshObjectivesList();
}

void ObjectivesEditor::_onMoveDownObjective()
{
	// get the current index
	int index = (*_curObjective)[_objectiveColumns.objNumber];

	// Pass the call to the general method
	_curEntity->second->moveObjective(index, +1);

	refreshObjectivesList();
}

// Delete an objective
void ObjectivesEditor::_onDeleteObjective()
{
	// Get the index of the current objective
	int index = (*_curObjective)[_objectiveColumns.objNumber];

	// Tell the ObjectiveEntity to delete this objective
	_curEntity->second->deleteObjective(index);

	// Repopulate the objective list
	refreshObjectivesList();
}

// Clear the objectives
void ObjectivesEditor::_onClearObjectives()
{
	// Clear the entity and refresh the list
	_curEntity->second->clearObjectives();
	refreshObjectivesList();
}

void ObjectivesEditor::_onEditLogic()
{
	MissionLogicDialog _dialog(getRefPtr(), *_curEntity->second);
	_dialog.show();

	refreshObjectivesList();
}

void ObjectivesEditor::_onEditObjConditions()
{
	ObjectiveConditionsDialog _dialog(getRefPtr(), *_curEntity->second);
	_dialog.show();

	refreshObjectivesList();
}

} // namespace objectives
