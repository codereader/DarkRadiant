#include "OrthoContextMenu.h"

#include "i18n.h"
#include "selectionlib.h"
#include "ibrush.h"
#include "isound.h"
#include "ieventmanager.h"
#include "iresourcechooser.h"
#include "idialogmanager.h"
#include "entitylib.h" // EntityFindByClassnameWalker
#include "ientity.h" // Node_getEntity()
#include "iregistry.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "map/Map.h"
#include "modulesystem/ModuleRegistry.h"

#include "wxutil/dialog/MessageBox.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/menu/CommandMenuItem.h"

#include "selection/algorithm/Group.h"
#include "selection/algorithm/ModelFinder.h"
#include "selection/algorithm/Entity.h"
#include "ui/modelselector/ModelSelector.h"
#include "ui/entitychooser/EntityClassChooser.h"

#include "string/convert.h"
#include "math/AABB.h"

#include <wx/window.h>
#include <wx/menu.h>

#include "modulesystem/StaticModule.h"

namespace ui
{

namespace {

    /* CONSTANTS */

    const char* LIGHT_CLASSNAME = "light";
    const char* MODEL_CLASSNAME = "func_static";
    const char* SPEAKER_CLASSNAME = "speaker";
    const char* PLAYERSTART_CLASSNAME = "info_player_start";

    // Angle key for the player start
    const char* ANGLE_KEY_NAME = "angle";
    const char* DEFAULT_ANGLE = "90"; // north

    const char* ADD_ENTITY_TEXT = N_("Create entity...");
    const char* ADD_ENTITY_ICON = "cmenu_add_entity.png";
    const char* ADD_PLAYERSTART_TEXT = N_("Create player start here");
    const char* ADD_PLAYERSTART_ICON = "player_start16.png";
    const char* MOVE_PLAYERSTART_TEXT = N_("Move player start here");
    const char* MOVE_PLAYERSTART_ICON = "player_start16.png";
    const char* ADD_MODEL_TEXT = N_("Create model...");
    const char* ADD_MODEL_ICON = "cmenu_add_model.png";
    const char* ADD_MONSTERCLIP_TEXT = N_("Surround with monsterclip");
    const char* ADD_MONSTERCLIP_ICON = "monsterclip16.png";
    const char* ADD_LIGHT_TEXT = N_("Create light...");
    const char* ADD_LIGHT_ICON = "cmenu_add_light.png";
    const char* ADD_PREFAB_TEXT = N_("Insert prefab...");
    const char* ADD_PREFAB_ICON = "cmenu_add_prefab.png";
    const char* ADD_SPEAKER_TEXT = N_("Create speaker...");
    const char* ADD_SPEAKER_ICON = "icon_sound.png";
    const char* CONVERT_TO_STATIC_TEXT = N_("Convert to func_static");
    const char* CONVERT_TO_STATIC_ICON = "cmenu_convert_static.png";
    const char* REPARENT_PRIMITIVES_TEXT = N_("Reparent primitives");
    const char* REVERT_TO_WORLDSPAWN_TEXT = N_("Revert to worldspawn");
    const char* REVERT_TO_WORLDSPAWN_PARTIAL_TEXT = N_("Revert part to worldspawn");
    const char* REVERT_TO_WORLDSPAWN_ICON = "cmenu_revert_worldspawn.png";
    const char* MERGE_ENTITIES_ICON = "cmenu_merge_entities.png";
    const char* MERGE_ENTITIES_TEXT = N_("Merge Entities");
    const char* MAKE_VISPORTAL = N_("Make Visportal");
    const char* MAKE_VISPORTAL_ICON = "make_visportal.png";
}

// Define the static OrthoContextMenu module
module::StaticModule<OrthoContextMenu> orthoContextMenuModule;

OrthoContextMenu& OrthoContextMenu::Instance()
{
    return *orthoContextMenuModule.getModule();
}

// Constructor
OrthoContextMenu::OrthoContextMenu()
{}

// Show the menu

void OrthoContextMenu::Show(wxWindow* parent, const Vector3& point)
{
    _lastPoint = point;

    analyseSelection();

    // Perform the visibility/sensitivity tests
    for (MenuSections::const_iterator sec = _sections.begin(); sec != _sections.end(); ++sec)
    {
        for (MenuItems::const_iterator i = sec->second.begin(); i != sec->second.end(); ++i)
        {
            ui::IMenuItem& item = *(*i);

            bool visible = item.isVisible();

            if (visible)
            {
                // Visibility check passed
                // Run the preshow command
                item.preShow();

                item.getMenuItem()->Enable(item.isSensitive());
            }
            else
            {
                // Visibility check failed, skip sensitivity check
                item.getMenuItem()->Enable(false);
            }
        }
    }

	parent->PopupMenu(_widget.get());
}

void OrthoContextMenu::analyseSelection()
{
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    bool anythingSelected = info.totalCount > 0;
    bool noEntities = info.entityCount == 0;
    bool noComponents = info.componentCount == 0;

    _selectionInfo.anythingSelected = anythingSelected;
    _selectionInfo.onlyPrimitivesSelected = anythingSelected && noEntities && noComponents;
    _selectionInfo.onlyBrushesSelected = anythingSelected && info.totalCount == info.brushCount;
    _selectionInfo.onlyPatchesSelected = anythingSelected && info.totalCount == info.patchCount;
    _selectionInfo.singlePrimitiveSelected = info.totalCount == 1 && (info.brushCount + info.patchCount == 1);

    _selectionInfo.onlyEntitiesSelected = anythingSelected && info.totalCount == info.entityCount;

    if (_selectionInfo.onlyEntitiesSelected)
    {
        // Check for group nodes
        selection::algorithm::GroupNodeChecker walker;
        GlobalSelectionSystem().foreachSelected(walker);

        _selectionInfo.onlyGroupsSelected = walker.onlyGroupsAreSelected();
        _selectionInfo.singleGroupSelected = walker.selectedGroupCount() == 1 && !Node_isWorldspawn(walker.getFirstSelectedGroupNode());

        // Create a ModelFinder and check whether only models were selected
        selection::algorithm::ModelFinder visitor;
        GlobalSelectionSystem().foreachSelected(visitor);

        // enable the "Add MonsterClip" entry only if at least one model is selected
        _selectionInfo.onlyModelsSelected = !visitor.empty() && visitor.onlyModels();
    }
    else
    {
        _selectionInfo.onlyGroupsSelected = false;
        _selectionInfo.singleGroupSelected = false;
        _selectionInfo.onlyModelsSelected = false;
    }

    // Check if a playerStart already exists
    EntityNodeFindByClassnameWalker walker(PLAYERSTART_CLASSNAME);
    GlobalSceneGraph().root()->traverse(walker);

    _selectionInfo.playerStartExists = walker.getEntity() != NULL;
}

bool OrthoContextMenu::checkMakeVisportal()
{
    return _selectionInfo.onlyBrushesSelected;
}

// Check if the convert to static command should be enabled
bool OrthoContextMenu::checkConvertStatic()
{
    // Command should be enabled if there is at least one selected
    // primitive and no non-primitive selections.
    return _selectionInfo.onlyPrimitivesSelected;
}

bool OrthoContextMenu::checkAddMonsterclip()
{
    return _selectionInfo.onlyModelsSelected;
}

bool OrthoContextMenu::checkAddEntity()
{
    return !_selectionInfo.anythingSelected || _selectionInfo.onlyPrimitivesSelected;
}

bool OrthoContextMenu::checkAddModel()
{
    return !_selectionInfo.anythingSelected;
}

bool OrthoContextMenu::checkAddPlayerStart()
{
    return !_selectionInfo.anythingSelected && !_selectionInfo.playerStartExists;
}

bool OrthoContextMenu::checkMovePlayerStart()
{
    return !_selectionInfo.anythingSelected && _selectionInfo.playerStartExists;
}

bool OrthoContextMenu::checkRevertToWorldspawn()
{
    return _selectionInfo.onlyGroupsSelected;
}

bool OrthoContextMenu::checkMergeEntities()
{
    return _selectionInfo.onlyGroupsSelected && !_selectionInfo.singleGroupSelected;
}

bool OrthoContextMenu::checkReparentPrimitives()
{
    return selection::algorithm::curSelectionIsSuitableForReparent();
}

bool OrthoContextMenu::checkRevertToWorldspawnPartial()
{
    if (_selectionInfo.singlePrimitiveSelected)
    {
        // Check the selected node
        scene::INodePtr node = GlobalSelectionSystem().ultimateSelected();

        // Only allow revert partial if the parent node is a groupnode
        if (node != NULL && Node_isPrimitive(node))
        {
            scene::INodePtr parent = node->getParent();

            return parent != NULL && scene::hasChildPrimitives(parent) && !Node_isWorldspawn(parent);
        }
    }

    return false;
}

// Get a registry key with a default value
std::string OrthoContextMenu::getRegistryKeyWithDefault(
    const std::string& key, const std::string& defaultVal
)
{
    std::string value = GlobalRegistry().get(key);
    if (value.empty())
        return defaultVal;
    else
        return value;
}

void OrthoContextMenu::addEntity()
{
    UndoableCommand command("createEntity");

    // Display the chooser to select an entity classname
    std::string cName = EntityClassChooser::chooseEntityClass();

    if (!cName.empty()) 
    {
        // Create the entity. We might get an EntityCreationException if the
        // wrong number of brushes is selected.
        try
		{
            selection::algorithm::createEntityFromSelection(cName, _lastPoint);
        }
        catch (selection::algorithm::EntityCreationException& e)
		{
            wxutil::Messagebox::ShowError(e.what());
        }
    }
}

void OrthoContextMenu::addPlayerStart()
{
    UndoableCommand command("addPlayerStart");

    try
    {
        // Create the player start entity
        scene::INodePtr playerStartNode = selection::algorithm::createEntityFromSelection(
            PLAYERSTART_CLASSNAME, _lastPoint
        );
        Entity* playerStart = Node_getEntity(playerStartNode);

        // Set a default angle
        playerStart->setKeyValue(ANGLE_KEY_NAME, DEFAULT_ANGLE);
    }
    catch (selection::algorithm::EntityCreationException& e) {
        wxutil::Messagebox::ShowError(e.what());
    }
}

void OrthoContextMenu::callbackMovePlayerStart()
{
    UndoableCommand _cmd("movePlayerStart");

    EntityNodeFindByClassnameWalker walker(PLAYERSTART_CLASSNAME);
    GlobalSceneGraph().root()->traverse(walker);

    Entity* playerStart = walker.getEntity();

    if (playerStart != NULL)
    {
        playerStart->setKeyValue("origin", string::to_string(_lastPoint));
    }
}

void OrthoContextMenu::callbackAddLight()
{
    UndoableCommand command("addLight");

    try 
	{
        selection::algorithm::createEntityFromSelection(LIGHT_CLASSNAME, _lastPoint);
    }
    catch (selection::algorithm::EntityCreationException&)
	{
        wxutil::Messagebox::ShowError(_("Unable to create light, classname not found."));
    }
}

void OrthoContextMenu::callbackAddPrefab()
{
    // Pass the call to the map algorithm and give the lastPoint coordinate as argument
    GlobalMap().loadPrefabAt(_lastPoint);
}

void OrthoContextMenu::callbackAddSpeaker()
{
    UndoableCommand command("addSpeaker");

    // Cancel all selection
    GlobalSelectionSystem().setSelectedAll(false);

    // Create the speaker entity
    scene::INodePtr spkNode;

    try
    {
        spkNode = selection::algorithm::createEntityFromSelection(
            SPEAKER_CLASSNAME, _lastPoint
        );
    }
    catch (selection::algorithm::EntityCreationException&) {
        wxutil::Messagebox::ShowError(_("Unable to create speaker, classname not found."));
        return;
    }

    // Initialise the speaker with suitable values
    if (module::ModuleRegistry::Instance().moduleExists(MODULE_SOUNDMANAGER))
    {
		IResourceChooser* chooser = GlobalDialogManager().createSoundShaderChooser();

		// Use a SoundChooser dialog to get a selection from the user
		std::string soundShader = chooser->chooseResource();

		chooser->destroyDialog();

        if (soundShader.empty())
        {
            return;
        }

        // Set the keyvalue
        Entity* entity = Node_getEntity(spkNode);
        assert(entity);
        entity->setKeyValue("s_shader", soundShader);

        // Set initial radii according to values in sound shader
        ISoundShaderPtr snd = GlobalSoundManager().getSoundShader(soundShader);
        assert(snd);

        SoundRadii radii = snd->getRadii();
        entity->setKeyValue(
            "s_mindistance", string::to_string(radii.getMin(true))
        );
        entity->setKeyValue(
            "s_maxdistance",
            (radii.getMax() > 0
             ? string::to_string(radii.getMax(true)) : "10")
        );
    }
}

void OrthoContextMenu::callbackAddModel()
{
    UndoableCommand command("addModel");

    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    // To create a model selection must be empty
    if (info.totalCount == 0)
    {
        // Display the model selector and block waiting for a selection (may be empty)
        ModelSelectorResult ms = ui::ModelSelector::chooseModel("", true, true);

        // If a model was selected, create the entity and set its model key
        if (!ms.model.empty())
        {
            try {
                scene::INodePtr modelNode = selection::algorithm::createEntityFromSelection(
                    MODEL_CLASSNAME,
                    _lastPoint
                );

                //Node_getTraversable(GlobalSceneGraph().root())->insert(modelNode);
                Node_getEntity(modelNode)->setKeyValue("model", ms.model);
                Node_getEntity(modelNode)->setKeyValue("skin", ms.skin);

                // If 'createClip' is ticked, create a clip brush
                if (ms.createClip)
                {
                    GlobalCommandSystem().execute("SurroundWithMonsterclip");
                }
            }
            catch (selection::algorithm::EntityCreationException&)
            {
                wxutil::Messagebox::ShowError(_("Unable to create model, classname not found."));
            }
        }

    }
    else
    {
        wxutil::Messagebox::ShowError(
            _("Nothing must be selected for model creation")
        );
    }
}

void OrthoContextMenu::registerDefaultItems()
{
    wxutil::MenuItemPtr addEntity(
        new wxutil::MenuItem(
            new wxutil::IconTextMenuItem(_(ADD_ENTITY_TEXT), ADD_ENTITY_ICON),
            std::bind(&OrthoContextMenu::addEntity, this),
            std::bind(&OrthoContextMenu::checkAddEntity, this))
    );

    wxutil::MenuItemPtr addLight(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(ADD_LIGHT_TEXT), ADD_LIGHT_ICON),
            std::bind(&OrthoContextMenu::callbackAddLight, this),
            std::bind(&OrthoContextMenu::checkAddEntity, this)) // same as create entity
    );

    wxutil::MenuItemPtr addPrefab(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(ADD_PREFAB_TEXT), ADD_PREFAB_ICON),
            std::bind(&OrthoContextMenu::callbackAddPrefab, this),
            std::bind(&OrthoContextMenu::checkAddEntity, this)) // same as create entity
    );

    wxutil::MenuItemPtr addSpeaker(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(ADD_SPEAKER_TEXT), ADD_SPEAKER_ICON),
            std::bind(&OrthoContextMenu::callbackAddSpeaker, this),
            std::bind(&OrthoContextMenu::checkAddEntity, this)) // same as create entity
    );

    wxutil::MenuItemPtr addModel(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(ADD_MODEL_TEXT), ADD_MODEL_ICON),
            std::bind(&OrthoContextMenu::callbackAddModel, this),
            std::bind(&OrthoContextMenu::checkAddModel, this))
    );

    wxutil::CommandMenuItemPtr surroundWithMonsterClip(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(ADD_MONSTERCLIP_TEXT), ADD_MONSTERCLIP_ICON),
            "SurroundWithMonsterclip",
            std::bind(&OrthoContextMenu::checkAddMonsterclip, this),
            std::bind(&OrthoContextMenu::checkAddMonsterclip, this))
    );

    wxutil::MenuItemPtr addPlayerStart(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(ADD_PLAYERSTART_TEXT), ADD_PLAYERSTART_ICON),
            std::bind(&OrthoContextMenu::addPlayerStart, this),
            std::bind(&OrthoContextMenu::checkAddPlayerStart, this),
            std::bind(&OrthoContextMenu::checkAddPlayerStart, this))
    );

    wxutil::MenuItemPtr movePlayerStart(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(MOVE_PLAYERSTART_TEXT), MOVE_PLAYERSTART_ICON),
            std::bind(&OrthoContextMenu::callbackMovePlayerStart, this),
            std::bind(&OrthoContextMenu::checkMovePlayerStart, this))
    );

    wxutil::CommandMenuItemPtr convertStatic(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(CONVERT_TO_STATIC_TEXT), CONVERT_TO_STATIC_ICON),
            "ConvertSelectedToFuncStatic",
            std::bind(&OrthoContextMenu::checkConvertStatic, this))
    );

    wxutil::CommandMenuItemPtr revertWorldspawn(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(REVERT_TO_WORLDSPAWN_TEXT), REVERT_TO_WORLDSPAWN_ICON),
            "RevertToWorldspawn",
            std::bind(&OrthoContextMenu::checkRevertToWorldspawn, this))
    );

    wxutil::CommandMenuItemPtr mergeEntities(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(MERGE_ENTITIES_TEXT), MERGE_ENTITIES_ICON),
            "MergeSelectedEntities",
            std::bind(&OrthoContextMenu::checkMergeEntities, this))
    );

    wxutil::CommandMenuItemPtr revertToWorldspawnPartial(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(REVERT_TO_WORLDSPAWN_PARTIAL_TEXT), REVERT_TO_WORLDSPAWN_ICON),
            "ParentSelectionToWorldspawn",
            std::bind(&OrthoContextMenu::checkRevertToWorldspawnPartial, this))
    );

    wxutil::CommandMenuItemPtr reparentPrimitives(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(REPARENT_PRIMITIVES_TEXT), CONVERT_TO_STATIC_ICON),
            "ParentSelection",
            std::bind(&OrthoContextMenu::checkReparentPrimitives, this))
    );

    wxutil::CommandMenuItemPtr makeVisportal(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(MAKE_VISPORTAL), MAKE_VISPORTAL_ICON),
            "MakeVisportal",
            std::bind(&OrthoContextMenu::checkMakeVisportal, this))
    );

    // Register all constructed items
    addItem(addEntity, SECTION_CREATE);
    addItem(addModel, SECTION_CREATE);
    addItem(addLight, SECTION_CREATE);
    addItem(addSpeaker, SECTION_CREATE);
    addItem(addPrefab, SECTION_CREATE);

    addItem(addPlayerStart, SECTION_ACTION);
    addItem(movePlayerStart, SECTION_ACTION);
    addItem(convertStatic, SECTION_ACTION);
    addItem(revertWorldspawn, SECTION_ACTION);
    addItem(revertToWorldspawnPartial, SECTION_ACTION);
    addItem(mergeEntities, SECTION_ACTION);
    addItem(reparentPrimitives, SECTION_ACTION);
    addItem(makeVisportal, SECTION_ACTION);
    addItem(surroundWithMonsterClip, SECTION_ACTION);
}

void OrthoContextMenu::onItemClick(wxCommandEvent& ev)
{
	int commandId = ev.GetId();

	// Find the menu item with that ID
	for (MenuSections::const_iterator sec = _sections.begin(); sec != _sections.end(); ++sec)
    {
		for (MenuItems::const_iterator i = sec->second.begin();
			 i != sec->second.end(); ++i)
		{
			ui::IMenuItem& item = *(*i);

			if (item.getMenuItem()->GetId() == commandId)
			{
				item.execute();
				break;
			}
		}
    }

	ev.Skip();
}

void OrthoContextMenu::constructMenu()
{
    _widget.reset(new wxMenu);

    // Add all sections to menu

    addSectionItems(SECTION_CREATE, true); // no spacer for first category
    addSectionItems(SECTION_ACTION);
	addSectionItems(SECTION_SELECTION_GROUPS);
    addSectionItems(SECTION_LAYER);

    // Add the rest of the sections
    for (MenuSections::const_iterator sec = _sections.lower_bound(SECTION_LAYER+1);
         sec != _sections.end(); ++sec)
    {
        addSectionItems(sec->first);
    }

	_widget->Connect(wxEVT_MENU, wxCommandEventHandler(OrthoContextMenu::onItemClick), NULL, this);
}

void OrthoContextMenu::addSectionItems(int section, bool noSpacer)
{
    if (_sections.find(section) == _sections.end()) return;

    const MenuItems& items = _sections[section];

    if (!noSpacer && !items.empty())
    {
		_widget->AppendSeparator();
    }

    for (MenuItems::const_iterator i = items.begin(); i != items.end(); ++i)
    {
		_widget->Append((*i)->getMenuItem());
    }
}

const std::string& OrthoContextMenu::getName() const
{
    static std::string _name("OrthoContextMenu");
    return _name;
}

const StringSet& OrthoContextMenu::getDependencies() const
{
    static StringSet _dependencies;

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_UIMANAGER);
        _dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_EVENTMANAGER);
        _dependencies.insert(MODULE_RADIANT);
    }

    return _dependencies;
}

void OrthoContextMenu::initialiseModule(const ApplicationContext& ctx)
{
    GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(this, &OrthoContextMenu::constructMenu)
    );

    registerDefaultItems();
}

void OrthoContextMenu::shutdownModule()
{
	_sections.clear();
    _widget.reset();
}

void OrthoContextMenu::addItem(const IMenuItemPtr& item, int section)
{
    // Create section if not existing
    if (_sections.find(section) == _sections.end())
    {
        _sections.insert(MenuSections::value_type(section, MenuSections::mapped_type()));
    }

    _sections[section].push_back(item);
}

void OrthoContextMenu::removeItem(const IMenuItemPtr& item)
{
    for (MenuSections::iterator sec = _sections.begin(); sec != _sections.end(); ++sec)
    {
        MenuItems::iterator found = std::find(sec->second.begin(), sec->second.end(), item);

        if (found != sec->second.end())
        {
            sec->second.erase(found);

            // Remove empty sections
            if (sec->second.empty())
            {
                _sections.erase(sec);
            }

            break;
        }
    }
}

} // namespace ui
