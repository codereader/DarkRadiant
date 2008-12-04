#include "OrthoContextMenu.h"

#include "selectionlib.h"
#include "scenelib.h"
#include "ibrush.h"
#include "entitylib.h" // EntityFindByClassnameWalker
#include "entity.h" // Entity_createFromSelection()
#include "ientity.h" // Node_getEntity()
#include "iregistry.h"
#include "mainframe.h"
#include "map/Map.h"

#include "gtkutil/dialog.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/TextMenuItem.h"

#include "selection/algorithm/Group.h"
#include "selection/algorithm/ModelFinder.h"
#include "ui/modelselector/ModelSelector.h"
#include "ui/entitychooser/EntityClassChooser.h"
#include "ui/layers/LayerContextMenu.h"

#include "math/aabb.h"
#include "brushmanip.h"

namespace ui
{
	
namespace {

    /* CONSTANTS */
    
    const char* LIGHT_CLASSNAME = "light";
    const char* MODEL_CLASSNAME = "func_static";
    const char* SPEAKER_CLASSNAME = "speaker";
    const char* PLAYERSTART_CLASSNAME = "info_player_start";

	const std::string RKEY_MONSTERCLIP_SHADER = "game/defaults/monsterClipShader";

    const char* ADD_ENTITY_TEXT = "Create entity...";
    const char* ADD_ENTITY_ICON = "cmenu_add_entity.png";
    const char* ADD_PLAYERSTART_TEXT = "Create player start here";
    const char* ADD_PLAYERSTART_ICON = "player_start16.png";
    const char* MOVE_PLAYERSTART_TEXT = "Move player start here";
    const char* MOVE_PLAYERSTART_ICON = "player_start16.png";
    const char* ADD_MODEL_TEXT = "Create model...";
    const char* ADD_MODEL_ICON = "cmenu_add_model.png";
    const char* ADD_MONSTERCLIP_TEXT = "Surround with monsterclip";
    const char* ADD_MONSTERCLIP_ICON = "monsterclip16.png";
    const char* ADD_LIGHT_TEXT = "Create light...";
    const char* ADD_LIGHT_ICON = "cmenu_add_light.png";
    const char* ADD_PREFAB_TEXT = "Insert prefab...";
    const char* ADD_PREFAB_ICON = "cmenu_add_prefab.png";
    const char* ADD_SPEAKER_TEXT = "Create speaker...";
    const char* ADD_SPEAKER_ICON = "icon_sound.png";
    const char* CONVERT_TO_STATIC_TEXT = "Convert to func_static";
    const char* CONVERT_TO_STATIC_ICON = "cmenu_convert_static.png";
    const char* REVERT_TO_WORLDSPAWN_TEXT = "Revert to worldspawn";
    const char* REVERT_TO_WORLDSPAWN_ICON = "cmenu_revert_worldspawn.png";

	const char* LAYER_ICON = "layers.png";
	const char* ADD_TO_LAYER_TEXT = "Add to Layer...";
	const char* MOVE_TO_LAYER_TEXT = "Move to Layer...";
	const char* REMOVE_FROM_LAYER_TEXT = "Remove from Layer...";
}

// Static class function to display the singleton instance.

void OrthoContextMenu::displayInstance(const Vector3& point) {
	static OrthoContextMenu _instance;
	_instance.show(point);
}

// Constructor. Create GTK widgets here.

OrthoContextMenu::OrthoContextMenu()
: _widget(gtk_menu_new())
{
	_addEntity = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_ENTITY_ICON), ADD_ENTITY_TEXT);
	_addPlayerStart = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_PLAYERSTART_ICON), ADD_PLAYERSTART_TEXT);
	_movePlayerStart = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(MOVE_PLAYERSTART_ICON), MOVE_PLAYERSTART_TEXT);
	_addModel = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_MODEL_ICON), ADD_MODEL_TEXT);
	_addMonsterClip = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_MONSTERCLIP_ICON), ADD_MONSTERCLIP_TEXT);
	_addLight = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_LIGHT_ICON), ADD_LIGHT_TEXT);
	_addPrefab = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_PREFAB_ICON), ADD_PREFAB_TEXT);
	_addSpkr = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(ADD_SPEAKER_ICON), ADD_SPEAKER_TEXT);
	_convertStatic = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(CONVERT_TO_STATIC_ICON), CONVERT_TO_STATIC_TEXT);
	_revertWorldspawn = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(REVERT_TO_WORLDSPAWN_ICON), REVERT_TO_WORLDSPAWN_TEXT);

	// "Add to layer" submenu
	_addToLayer = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(LAYER_ICON), ADD_TO_LAYER_TEXT);
	_moveToLayer = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(LAYER_ICON), MOVE_TO_LAYER_TEXT);
	_removeFromLayer = gtkutil::IconTextMenuItem(GlobalRadiant().getLocalPixbuf(LAYER_ICON), REMOVE_FROM_LAYER_TEXT);
	
	g_signal_connect(G_OBJECT(_addEntity), "activate", G_CALLBACK(callbackAddEntity), this);
	g_signal_connect(G_OBJECT(_addPlayerStart), "activate", G_CALLBACK(callbackAddPlayerStart), this);
	g_signal_connect(G_OBJECT(_movePlayerStart), "activate", G_CALLBACK(callbackMovePlayerStart), this);
	g_signal_connect(G_OBJECT(_addModel), "activate", G_CALLBACK(callbackAddModel), this);
	g_signal_connect(G_OBJECT(_addMonsterClip), "activate", G_CALLBACK(callbackAddMonsterClip), this);
	g_signal_connect(G_OBJECT(_addLight), "activate", G_CALLBACK(callbackAddLight), this);
	g_signal_connect(G_OBJECT(_addPrefab), "activate", G_CALLBACK(callbackAddPrefab), this);
	g_signal_connect(G_OBJECT(_addSpkr), "activate", G_CALLBACK(callbackAddSpeaker), this);
	g_signal_connect(G_OBJECT(_convertStatic), "activate", G_CALLBACK(callbackConvertToStatic), this);
	g_signal_connect(G_OBJECT(_revertWorldspawn), "activate", G_CALLBACK(callbackRevertToWorldspawn), this);

	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addEntity);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addModel);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addLight);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addSpkr);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addPrefab);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addMonsterClip);
    gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addPlayerStart);
    gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _movePlayerStart);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _convertStatic);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _revertWorldspawn);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _addToLayer);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _moveToLayer);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _removeFromLayer);

	gtk_widget_show_all(_widget);
}

// Show the menu

void OrthoContextMenu::show(const Vector3& point) {
	_lastPoint = point;
	checkConvertStatic(); // enable or disable the convert-to-static command
	checkRevertToWorldspawn();
	checkMonsterClip(); // enable the "Add MonsterClip" entry only if one or more model is selected
	checkPlayerStart(); // change the "Add PlayerStart" entry if an info_player_start is already existant 
	checkAddOptions(); // disable the "Add *" command if an entity is already selected
	repopulateLayerMenus(); // refresh the layer menus
	gtk_menu_popup(GTK_MENU(_widget), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

void OrthoContextMenu::repopulateLayerMenus() {
	// Create a new submenu and connect it to the according function
	LayerContextMenu::OnSelectionFunc addToLayerCallback(callbackAddToLayer);
	LayerContextMenu::OnSelectionFunc moveToLayerCallback(callbackMoveToLayer);
	LayerContextMenu::OnSelectionFunc removeFromLayerCallback(callbackRemoveFromLayer);

	// Create a new LayerContextMenu
	_addToLayerSubmenu = LayerContextMenuPtr(new LayerContextMenu(addToLayerCallback));
	_moveToLayerSubmenu = LayerContextMenuPtr(new LayerContextMenu(moveToLayerCallback));
	_removeFromLayerSubmenu = LayerContextMenuPtr(new LayerContextMenu(removeFromLayerCallback));

	// Cast the LayerContextMenu onto GtkWidget* and pack it
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(_addToLayer), *_addToLayerSubmenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(_moveToLayer), *_moveToLayerSubmenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(_removeFromLayer), *_removeFromLayerSubmenu);

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	gtk_widget_set_sensitive(_addToLayer, info.totalCount > 0);
	gtk_widget_set_sensitive(_moveToLayer, info.totalCount > 0);
	gtk_widget_set_sensitive(_removeFromLayer, info.totalCount > 0);
}

// Check if the convert to static command should be enabled
void OrthoContextMenu::checkConvertStatic() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	// Command should be enabled if there is at least one selected
	// primitive and no non-primitive selections.
	gtk_widget_set_sensitive(
		_convertStatic, 
		(info.entityCount == 0 && info.componentCount == 0 && info.totalCount > 0) 
	);
}

void OrthoContextMenu::checkMonsterClip() {

	// create a ModelFinder and check whether only models were selected
	selection::algorithm::ModelFinder visitor;
	GlobalSelectionSystem().foreachSelected(visitor);

	// enable the "Add MonsterClip" entry only if one or more model is selected
	gtk_widget_set_sensitive(
		_addMonsterClip, 
		!visitor.empty() && visitor.onlyModels()
	);
}

void OrthoContextMenu::checkAddOptions() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	// Add entity, playerStart and light commands are disabled if an entity is selected
	gtk_widget_set_sensitive(_addEntity, info.entityCount == 0 ? TRUE : FALSE);
	gtk_widget_set_sensitive(_addPlayerStart, info.entityCount == 0 ? TRUE : FALSE);
	gtk_widget_set_sensitive(_movePlayerStart, info.entityCount == 0 ? TRUE : FALSE);
	gtk_widget_set_sensitive(_addLight, info.entityCount == 0 ? TRUE : FALSE);
		
	// Add speaker/model is disabled if anything is selected
	gtk_widget_set_sensitive(_addSpkr, info.totalCount == 0 ? TRUE : FALSE);
	gtk_widget_set_sensitive(_addModel, info.totalCount == 0 ? TRUE : FALSE);
}

void OrthoContextMenu::checkPlayerStart() {
	// Check if a playerStart already exists
	EntityFindByClassnameWalker walker(PLAYERSTART_CLASSNAME);
	GlobalSceneGraph().traverse(walker);
	
	if (walker.getEntity() == NULL) {
		gtk_widget_hide(_movePlayerStart);
		gtk_widget_show(_addPlayerStart);
	}
	else {
		gtk_widget_hide(_addPlayerStart);
		gtk_widget_show(_movePlayerStart);
	}
}

void OrthoContextMenu::checkRevertToWorldspawn() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	bool sensitive = false;
	
	// Only entities are allowed to be selected, but they have to be groupnodes
	if (info.totalCount > 0 && info.totalCount == info.entityCount) {

		// Check the selection using a local walker
		class GroupNodeChecker : 
			public SelectionSystem::Visitor
		{
			mutable bool _onlyGroups;
		public:
			GroupNodeChecker() :
				_onlyGroups(true)
			{}

			void visit(const scene::INodePtr& node) const {
				if (!node_is_group(node)) {
					_onlyGroups = false;
				}
			}

			bool onlyGroupsAreSelected() const {
				return _onlyGroups;
			}
		};
		
		GroupNodeChecker walker;
		GlobalSelectionSystem().foreachSelected(walker);

		sensitive = walker.onlyGroupsAreSelected();
	}
	
	if (sensitive) {
		gtk_widget_set_sensitive(_revertWorldspawn, true);
		gtk_widget_show_all(_revertWorldspawn);
		gtk_widget_hide_all(_convertStatic);
	}
	else {
		gtk_widget_set_sensitive(_revertWorldspawn, false);
		gtk_widget_hide_all(_revertWorldspawn);
		gtk_widget_show_all(_convertStatic);
	}
}

/* GTK CALLBACKS */

void OrthoContextMenu::callbackAddEntity(GtkMenuItem* item, 
										 OrthoContextMenu* self) 
{
	UndoableCommand command("createEntity");

	// Display the chooser to select an entity classname
	std::string cName = EntityClassChooser::chooseEntityClass();
	
	if (!cName.empty()) {
		// Create the entity. We might get an EntityCreationException if the 
		// wrong number of brushes is selected.
		try {
			Entity_createFromSelection(cName.c_str(), self->_lastPoint);
		}
		catch (EntityCreationException e) {
			gtkutil::errorDialog(e.what(), GlobalRadiant().getMainWindow());
		}
	}
}

void OrthoContextMenu::callbackAddPlayerStart(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("addPlayerStart");	

	try {
		Entity_createFromSelection(PLAYERSTART_CLASSNAME, self->_lastPoint);
	}
	catch (EntityCreationException e) {
		gtkutil::errorDialog(e.what(), GlobalRadiant().getMainWindow());
	}
}

void OrthoContextMenu::callbackMovePlayerStart(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand _cmd("movePlayerStart");
	
	EntityFindByClassnameWalker walker(PLAYERSTART_CLASSNAME);
	GlobalSceneGraph().traverse(walker);
	
	Entity* playerStart = walker.getEntity();
	
	if (playerStart != NULL) {
		playerStart->setKeyValue("origin", self->_lastPoint);
	}
}

void OrthoContextMenu::callbackAddMonsterClip(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("addMonsterclip");	

	// create a ModelFinder and retrieve the modelList
	selection::algorithm::ModelFinder visitor;
	GlobalSelectionSystem().foreachSelected(visitor);

	// Retrieve the list with all the found models from the visitor
	selection::algorithm::ModelFinder::ModelList list = visitor.getList();
	
	selection::algorithm::ModelFinder::ModelList::iterator iter;
	for (iter = list.begin(); iter != list.end(); ++iter) {
		// one of the models in the SelectionStack
		const scene::INodePtr& node = *iter;

		// retrieve the AABB
		AABB brushAABB(node->worldAABB()); // TODO: Check if worldAABB() is appropriate

		// create the brush
		scene::INodePtr brushNode(GlobalBrushCreator().createBrush());

		if (brushNode != NULL) {
			scene::addNodeToContainer(brushNode, GlobalMap().findOrInsertWorldspawn());
			
			Brush* theBrush = Node_getBrush(brushNode);

			std::string clipShader = GlobalRegistry().get(RKEY_MONSTERCLIP_SHADER);

			Scene_BrushResize(*theBrush, brushAABB, clipShader);
		}
	}
}

void OrthoContextMenu::callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("addLight");	

    try {
    	Entity_createFromSelection(LIGHT_CLASSNAME, self->_lastPoint);
    }
    catch (EntityCreationException e) {
        gtkutil::errorDialog("Unable to create light, classname not found.",
                             GlobalRadiant().getMainWindow());
    }
}

void OrthoContextMenu::callbackAddPrefab(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("addPrefab");	

	// Pass the call to the map algorithm and give the lastPoint coordinate as argument
	GlobalMap().loadPrefabAt(self->_lastPoint);
}

void OrthoContextMenu::callbackAddSpeaker(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("addSpeaker");	

    try {
        Entity_createFromSelection(SPEAKER_CLASSNAME, self->_lastPoint);	
    }
    catch (EntityCreationException e) {
        gtkutil::errorDialog("Unable to create speaker, classname not found.",
                             GlobalRadiant().getMainWindow());
    }
}

void OrthoContextMenu::callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("addModel");	

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	// To create a model we need EITHER nothing selected OR exactly one brush selected.
	if (info.totalCount == 0 || info.brushCount == 1) {
		// Display the model selector and block waiting for a selection (may be empty)
		ModelSelectorResult ms = ui::ModelSelector::chooseModel();
		
		// If a model was selected, create the entity and set its model key
		if (!ms.model.empty()) {
			try {
				scene::INodePtr modelNode = Entity_createFromSelection(
					MODEL_CLASSNAME, 
					self->_lastPoint
				);
			
				//Node_getTraversable(GlobalSceneGraph().root())->insert(modelNode);
				Node_getEntity(modelNode)->setKeyValue("model", ms.model);
				Node_getEntity(modelNode)->setKeyValue("skin", ms.skin);

				// If 'createClip' is ticked, create a clip brush
				if (ms.createClip) {
					// get the model
					scene::Path modelPath(GlobalSceneGraph().root());
					modelPath.push(modelNode);
					
					// retrieve the AABB
					AABB brushAABB(modelNode->worldAABB()); // TODO: Check if worldAABB() is ok
	
					// create the brush
					scene::INodePtr brushNode(GlobalBrushCreator().createBrush());
					scene::addNodeToContainer(brushNode, GlobalMap().findOrInsertWorldspawn());
					Brush* theBrush = Node_getBrush(brushNode);
					std::string clipShader = GlobalRegistry().get(RKEY_MONSTERCLIP_SHADER);
					Scene_BrushResize(*theBrush, brushAABB, clipShader);
				}
            }
            catch (EntityCreationException e) {
                gtkutil::errorDialog("Unable to create model, classname not found.",
                                     GlobalRadiant().getMainWindow());
            }
		}
		
	}
	else {
		gtkutil::errorDialog(
            "Either nothing or exactly one brush must be selected for model creation",
			GlobalRadiant().getMainWindow()
        );
	}

}

void OrthoContextMenu::callbackConvertToStatic(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("convertToStatic");	

	// Create a func_static entity. Only brushes can be selected if this menu item is
	// enabled.
	Entity_createFromSelection(MODEL_CLASSNAME, self->_lastPoint);	
}

void OrthoContextMenu::callbackRevertToWorldspawn(GtkMenuItem* item, OrthoContextMenu* self) {
	UndoableCommand command("revertToWorldspawn");	

	// Pass the call to the according method
	selection::algorithm::revertGroupToWorldSpawn();	
}

void OrthoContextMenu::callbackAddToLayer(int layerID) {
	scene::getLayerSystem().addSelectionToLayer(layerID);
	GlobalMap().setModified(true);
}

void OrthoContextMenu::callbackMoveToLayer(int layerID) {
	scene::getLayerSystem().moveSelectionToLayer(layerID);
	GlobalMap().setModified(true);
}

void OrthoContextMenu::callbackRemoveFromLayer(int layerID) {
	scene::getLayerSystem().removeSelectionFromLayer(layerID);
	GlobalMap().setModified(true);
}

} // namespace ui
