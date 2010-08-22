#include "ObjectivesEditor.h"
#include "ObjectiveEntityFinder.h"
#include "RandomOrigin.h"
#include "TargetList.h"
#include "ComponentsDialog.h"
#include "MissionLogicDialog.h"
#include "util/ObjectivesException.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "imainframe.h"
#include "iregistry.h"
#include "ieclass.h"
#include "ientity.h"

#include "scenelib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog.h"

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
	const std::string RKEY_OBJECTIVE_ENTS = "game/objectivesEditor//objectivesEClass";
	
	inline std::string makeBold(const std::string& input)
	{
		return "<b>" + input + "</b>";
	}
}

// Constructor creates widgets
ObjectivesEditor::ObjectivesEditor() :
	gtkutil::BlockingTransientWindow(_(DIALOG_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_objectiveEntityList(Gtk::ListStore::create(_objEntityColumns)),
	_objectiveList(Gtk::ListStore::create(_objectiveColumns))
{
	// Window properties
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	set_border_width(12);
    
    // Window size
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());

	set_default_size(static_cast<int>(rect.get_width()/2), static_cast<int>(2*rect.get_height()/3));

    // Main dialog vbox
	Gtk::VBox* mainVbx = Gtk::manage(new Gtk::VBox(false, 12));
	mainVbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Objectives entities")))),
					   false, false, 0);
	mainVbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createEntitiesPanel(), 18, 1.0)),
					   false, false, 0);
	mainVbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Objectives")))),
					   false, false, 0);
	mainVbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createObjectivesPanel(), 18, 1.0)),
					   true, true, 0);
	mainVbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(makeBold(_("Success/Failure Logic")))),
					   false, false, 0);
	mainVbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createLogicPanel(), 18, 1.0)),
					   false, false, 0);
	mainVbx->pack_start(*Gtk::manage(new Gtk::HSeparator), false, false, 0);
	mainVbx->pack_end(createButtons(), false, false, 0);

	// Add vbox to dialog
	add(*mainVbx);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);
	
	_windowPosition.connect(this);
	_windowPosition.applyPosition();

	_objectiveEClasses.clear();

	xml::NodeList nodes = GlobalRegistry().findXPath(RKEY_OBJECTIVE_ENTS);

	for (xml::NodeList::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		_objectiveEClasses.push_back(i->getAttributeValue("name"));
	}
}

// Create the objects panel (for manipulating the target_addobjectives objects)
Gtk::Widget& ObjectivesEditor::createEntitiesPanel()
{
	// Hbox containing the entity list and the buttons vbox
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));
	
	// Tree view listing the target_addobjectives entities
	_entityList = Gtk::manage(new Gtk::TreeView(_objectiveEntityList));

	_entityList->set_headers_visible(false);

	_entityList->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ObjectivesEditor::_onEntitySelectionChanged));
	
	// Active-at-start column (checkbox)
	Gtk::CellRendererToggle* startToggle = Gtk::manage(new Gtk::CellRendererToggle);
	startToggle->signal_toggled().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onStartActiveCellToggled));
	
	Gtk::TreeViewColumn* startCol = Gtk::manage(new Gtk::TreeViewColumn(_("Start")));
	startCol->add_attribute(startToggle->property_active(), _objEntityColumns.startActive);
	
	_entityList->append_column(*startCol);
	
	// Name column
	_entityList->append_column(*Gtk::manage(new gtkutil::TextColumnmm("", _objEntityColumns.displayName)));
	
	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_entityList)), true, true, 0);
					   
	// Vbox for the buttons
	Gtk::VBox* buttonBox = Gtk::manage(new Gtk::VBox(false, 6));
	
	Gtk::Button* addButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD));
	addButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onAddEntity));

	_delEntityButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_delEntityButton->set_sensitive(false); // disabled at start
	_delEntityButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onDeleteEntity));
	
	buttonBox->pack_start(*addButton, true, true, 0);
	buttonBox->pack_start(*_delEntityButton, true, true, 0);
					   
	hbx->pack_start(*buttonBox, false, false, 0);

	return *hbx;
}

// Create the main objective editing widgets
Gtk::Widget& ObjectivesEditor::createObjectivesPanel()
{
	// Tree view
	_objList = Gtk::manage(new Gtk::TreeView(_objectiveList));
	_objList->set_headers_visible(true);

	_objList->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ObjectivesEditor::_onObjectiveSelectionChanged));
	
	// Key and value text columns
	_objList->append_column(*Gtk::manage(
		new gtkutil::TextColumnmm("#", _objectiveColumns.objNumber, false)));
	_objList->append_column(*Gtk::manage(
		new gtkutil::TextColumnmm(_("Description"), _objectiveColumns.description, false)));
	_objList->append_column(*Gtk::manage(
		new gtkutil::TextColumnmm(_("Diff."), _objectiveColumns.difficultyLevel, false)));
	
	// Beside the list is an vbox containing add, edit, delete and clear buttons
	_objButtonPanel = Gtk::manage(new Gtk::VBox(false, 6));
    
    // Buttons panel box is disabled by default, enabled once an Entity is
    // selected.
    _objButtonPanel->set_sensitive(false);

	Gtk::Button* addButton = Gtk::manage(new Gtk::Button(Gtk::Stock::ADD)); 
	addButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onAddObjective));

	_editObjButton = Gtk::manage(new Gtk::Button(Gtk::Stock::EDIT)); 
	_editObjButton->set_sensitive(false); // not enabled without selection 
	_editObjButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onEditObjective));

	_moveUpObjButton = Gtk::manage(new Gtk::Button(Gtk::Stock::GO_UP));
	_moveUpObjButton->set_sensitive(false); // not enabled without selection 
	_moveUpObjButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onMoveUpObjective));

	_moveDownObjButton = Gtk::manage(new Gtk::Button(Gtk::Stock::GO_DOWN));
	_moveDownObjButton->set_sensitive(false); // not enabled without selection 
	_moveDownObjButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onMoveDownObjective));

	_delObjButton = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
	_delObjButton->set_sensitive(false); // not enabled without selection 
	_delObjButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onDeleteObjective));
	
	_clearObjButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
	_clearObjButton->set_sensitive(false); // requires >0 objectives
	_clearObjButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onClearObjectives));
	
	_objButtonPanel->pack_start(*addButton, false, false, 0);
	_objButtonPanel->pack_start(*_editObjButton, false, false, 0);
	_objButtonPanel->pack_start(*_moveUpObjButton, false, false, 0);
	_objButtonPanel->pack_start(*_moveDownObjButton, false, false, 0);
	_objButtonPanel->pack_start(*_delObjButton, false, false, 0);
	_objButtonPanel->pack_start(*_clearObjButton, false, false, 0);

	// Pack the list and the buttons into an hbox
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 6));
	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_objList)), true, true, 0); 
	hbx->pack_start(*_objButtonPanel, false, false, 0);

	return *hbx; 
}

Gtk::Widget& ObjectivesEditor::createLogicPanel()
{
	_logicPanel = Gtk::manage(new Gtk::HBox(false, 6));
	_logicPanel->set_sensitive(false);

	Gtk::Button* editLogicButton = Gtk::manage(new Gtk::Button(_("Edit mission success/failure logic..."))); 
	editLogicButton->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::EDIT, Gtk::ICON_SIZE_BUTTON)));

	editLogicButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onEditLogic));

	_logicPanel->pack_start(*editLogicButton, true, true, 0);

	return *_logicPanel;
}

Gtk::Widget& ObjectivesEditor::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));

	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onCancel));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &ObjectivesEditor::_onOK));
	
	hbx->pack_end(*okButton, true, true, 0);
	hbx->pack_end(*cancelButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
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
	ObjectiveEntityFinder finder(_objectiveEntityList,
								 _objEntityColumns,
								 _entities, 
								 _objectiveEClasses);
	Node_traverseSubgraph(GlobalSceneGraph().root(), finder);
	
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
		gtkutil::errorDialog(
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
	if (_curEntity->second->isEmpty())
	{
		_clearObjButton->set_sensitive(false);
	}
	else
	{
		_clearObjButton->set_sensitive(true);
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
	
	// Get the selection
	Gtk::TreeModel::iterator iter = _entityList->get_selection()->get_selected();
	
	if (iter) 
    {
		// Get name of the entity and find the corresponding ObjectiveEntity in
		// the map
		std::string name = Glib::ustring((*iter)[_objEntityColumns.entityName]);
		
		// Save the current selection and refresh the objectives list
		_curEntity = _entities.find(name);
		refreshObjectivesList();
		
		// Enable the delete button and objectives panel
		_delEntityButton->set_sensitive(true);
		_logicPanel->set_sensitive(true);
        _objButtonPanel->set_sensitive(true);
	}
	else 
    {
		// No selection, disable the delete button and clear the objective
		// panel
		_delEntityButton->set_sensitive(false);
		_logicPanel->set_sensitive(false);
		_objButtonPanel->set_sensitive(false);
	}
}

// Callback for current objective selection changed
void ObjectivesEditor::_onObjectiveSelectionChanged()
{
	// Get the selection
	_curObjective = _objList->get_selection()->get_selected();

	if (_curObjective) 
    {
		// Enable the edit and delete buttons
		_editObjButton->set_sensitive(true);
		_delObjButton->set_sensitive(true);

		// Check if this is the first command in the list, get the ID of the selected item
		int index = (*_curObjective)[_objectiveColumns.objNumber];

		int highestIndex = _curEntity->second->getHighestObjIndex();
		int lowestIndex = _curEntity->second->getLowestObjIndex();

		bool hasNext = (highestIndex != -1 && highestIndex > index);
		bool hasPrev = (lowestIndex != -1 && lowestIndex < index);

		_moveUpObjButton->set_sensitive(hasPrev);
		_moveDownObjButton->set_sensitive(hasNext);
	}
	else 
    {
		// Disable the edit, delete and move buttons
		_editObjButton->set_sensitive(false);
		_delObjButton->set_sensitive(false);
		_moveUpObjButton->set_sensitive(false);
		_moveDownObjButton->set_sensitive(false);
	}
}

// Add a new objectives entity button
void ObjectivesEditor::_onAddEntity()
{
	if (_objectiveEClasses.empty())
	{
		// Objective entityclass(es) not defined
        gtkutil::errorDialog(
            _("Unable to create Objective Entity: classes not defined in registry."),
            GlobalMainFrame().getTopLevelWindow()
        );
		return;
	}

	const std::string& objEClass = _objectiveEClasses.front();

	// Obtain the entity class object
	IEntityClassPtr eclass = 
		GlobalEntityClassManager().findClass(objEClass);
		
    if (eclass) 
    {
        // Construct a Node of this entity type
        scene::INodePtr node(GlobalEntityCreator().createEntity(eclass));
        
        // Create a random offset
        Node_getEntity(node)->setKeyValue("origin", RandomOrigin::generate(128));
        
        // Insert the node into the scene graph
        assert(GlobalSceneGraph().root());
        GlobalSceneGraph().root()->addChildNode(node);
        
        // Refresh the widgets
        populateWidgets();
    }
    else 
    {
        // Objective entityclass was not found
        gtkutil::errorDialog(
			(boost::format(_("Unable to create Objective Entity: class '%s' not found.")) % objEClass).str(),
            GlobalMainFrame().getTopLevelWindow()
        );
    }
}

// Delete entity button
void ObjectivesEditor::_onDeleteEntity()
{
	// Get the selection
	Gtk::TreeModel::iterator iter = _entityList->get_selection()->get_selected();
	
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

} // namespace objectives
