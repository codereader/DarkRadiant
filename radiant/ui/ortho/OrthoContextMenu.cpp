#include "OrthoContextMenu.h"

#include "i18n.h"
#include "selectionlib.h"
#include "ibrush.h"
#include "isound.h"
#include "ieventmanager.h"
#include "entitylib.h" // EntityFindByClassnameWalker
#include "ientity.h" // Node_getEntity()
#include "iregistry.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "map/Map.h"
#include "modulesystem/ModuleRegistry.h"

#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/menu/CommandMenuItem.h"

#include "selection/algorithm/Group.h"
#include "selection/algorithm/ModelFinder.h"
#include "selection/algorithm/Entity.h"
#include "ui/modelselector/ModelSelector.h"
#include "ui/common/SoundChooser.h"
#include "ui/entitychooser/EntityClassChooser.h"

#include "math/AABB.h"

#include <gtkmm/menu.h>
#include <gtkmm/separatormenuitem.h>

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

void OrthoContextMenu::show(const Vector3& point)
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
                item.getWidget()->show();

                // Run the preshow command
                item.preShow();

                item.getWidget()->set_sensitive(item.isSensitive());
            }
            else
            {
                // Visibility check failed, skip sensitivity check
                item.getWidget()->hide();
            }
        }
    }

    _widget->popup(1, gtk_get_current_event_time());
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
        _selectionInfo.singleGroupSelected = walker.selectedGroupCount() == 1 && !node_is_worldspawn(walker.getFirstSelectedGroupNode());

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
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
    return info.totalCount == 0 || info.brushCount == 1;
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

            return parent != NULL && scene::isGroupNode(parent) && !node_is_worldspawn(parent);
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
            gtkutil::MessageBox::ShowError(e.what(), GlobalMainFrame().getTopLevelWindow());
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
        gtkutil::MessageBox::ShowError(e.what(), GlobalMainFrame().getTopLevelWindow());
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
        gtkutil::MessageBox::ShowError(_("Unable to create light, classname not found."),
                             GlobalMainFrame().getTopLevelWindow());
    }
}

void OrthoContextMenu::callbackAddPrefab()
{
    // Pass the call to the map algorithm and give the lastPoint coordinate as argument
    GlobalMap().loadPrefabAt(_lastPoint);
}

void OrthoContextMenu::callbackAddSpeaker()
{
    using boost::lexical_cast;

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
        gtkutil::MessageBox::ShowError(_("Unable to create speaker, classname not found."),
                             GlobalMainFrame().getTopLevelWindow());
        return;
    }

    // Initialise the speaker with suitable values
    if (module::ModuleRegistry::Instance().moduleExists(MODULE_SOUNDMANAGER))
    {
        // Display the Sound Chooser to get a sound shader from the user
        SoundChooser sChooser;
        sChooser.show();

        const std::string& soundShader = sChooser.getSelectedShader();
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
            "s_mindistance", lexical_cast<std::string>(radii.getMin(true))
        );
        entity->setKeyValue(
            "s_maxdistance",
            (radii.getMax() > 0
             ? lexical_cast<std
             ::string>(radii.getMax(true)) : "10")
        );
    }
}

void OrthoContextMenu::callbackAddModel()
{
    UndoableCommand command("addModel");

    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    // To create a model we need EITHER nothing selected OR exactly one brush selected.
    if (info.totalCount == 0 || info.brushCount == 1)
    {
        // Display the model selector and block waiting for a selection (may be empty)
        ModelSelectorResult ms = ui::ModelSelector::chooseModel("", true, true);

        // If a model was selected, create the entity and set its model key
        if (!ms.model.empty()) {
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
                gtkutil::MessageBox::ShowError(_("Unable to create model, classname not found."),
                                     GlobalMainFrame().getTopLevelWindow());
            }
        }

    }
    else
    {
        gtkutil::MessageBox::ShowError(
            _("Either nothing or exactly one brush must be selected for model creation"),
            GlobalMainFrame().getTopLevelWindow()
        );
    }
}

void OrthoContextMenu::registerDefaultItems()
{
    gtkutil::MenuItemPtr addEntity(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_ENTITY_ICON), _(ADD_ENTITY_TEXT))),
            boost::bind(&OrthoContextMenu::addEntity, this),
            boost::bind(&OrthoContextMenu::checkAddEntity, this))
    );

    gtkutil::MenuItemPtr addLight(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_LIGHT_ICON), _(ADD_LIGHT_TEXT))),
            boost::bind(&OrthoContextMenu::callbackAddLight, this),
            boost::bind(&OrthoContextMenu::checkAddEntity, this)) // same as create entity
    );

    gtkutil::MenuItemPtr addPrefab(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_PREFAB_ICON), _(ADD_PREFAB_TEXT))),
            boost::bind(&OrthoContextMenu::callbackAddPrefab, this),
            boost::bind(&OrthoContextMenu::checkAddEntity, this)) // same as create entity
    );

    gtkutil::MenuItemPtr addSpeaker(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_SPEAKER_ICON), _(ADD_SPEAKER_TEXT))),
            boost::bind(&OrthoContextMenu::callbackAddSpeaker, this),
            boost::bind(&OrthoContextMenu::checkAddEntity, this)) // same as create entity
    );

    gtkutil::MenuItemPtr addModel(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_MODEL_ICON), _(ADD_MODEL_TEXT))),
            boost::bind(&OrthoContextMenu::callbackAddModel, this),
            boost::bind(&OrthoContextMenu::checkAddModel, this))
    );

    gtkutil::CommandMenuItemPtr surroundWithMonsterClip(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_MONSTERCLIP_ICON), _(ADD_MONSTERCLIP_TEXT))),
            "SurroundWithMonsterclip",
            boost::bind(&OrthoContextMenu::checkAddMonsterclip, this),
            boost::bind(&OrthoContextMenu::checkAddMonsterclip, this))
    );

    gtkutil::MenuItemPtr addPlayerStart(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(ADD_PLAYERSTART_ICON), _(ADD_PLAYERSTART_TEXT))),
            boost::bind(&OrthoContextMenu::addPlayerStart, this),
            boost::bind(&OrthoContextMenu::checkAddPlayerStart, this),
            boost::bind(&OrthoContextMenu::checkAddPlayerStart, this))
    );

    gtkutil::MenuItemPtr movePlayerStart(
        new gtkutil::MenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(MOVE_PLAYERSTART_ICON), _(MOVE_PLAYERSTART_TEXT))),
            boost::bind(&OrthoContextMenu::callbackMovePlayerStart, this),
            boost::bind(&OrthoContextMenu::checkMovePlayerStart, this))
    );

    gtkutil::CommandMenuItemPtr convertStatic(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(CONVERT_TO_STATIC_ICON), _(CONVERT_TO_STATIC_TEXT))),
            "ConvertSelectedToFuncStatic",
            boost::bind(&OrthoContextMenu::checkConvertStatic, this))
    );

    gtkutil::CommandMenuItemPtr revertWorldspawn(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(REVERT_TO_WORLDSPAWN_ICON), _(REVERT_TO_WORLDSPAWN_TEXT))),
            "RevertToWorldspawn",
            boost::bind(&OrthoContextMenu::checkRevertToWorldspawn, this))
    );

    gtkutil::CommandMenuItemPtr mergeEntities(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(MERGE_ENTITIES_ICON), _(MERGE_ENTITIES_TEXT))),
            "MergeSelectedEntities",
            boost::bind(&OrthoContextMenu::checkMergeEntities, this))
    );

    gtkutil::CommandMenuItemPtr revertToWorldspawnPartial(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(REVERT_TO_WORLDSPAWN_ICON), _(REVERT_TO_WORLDSPAWN_PARTIAL_TEXT))),
            "ParentSelectionToWorldspawn",
            boost::bind(&OrthoContextMenu::checkRevertToWorldspawnPartial, this))
    );

    gtkutil::CommandMenuItemPtr reparentPrimitives(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(CONVERT_TO_STATIC_ICON), _(REPARENT_PRIMITIVES_TEXT))),
            "ParentSelection",
            boost::bind(&OrthoContextMenu::checkReparentPrimitives, this))
    );

    gtkutil::CommandMenuItemPtr makeVisportal(
        new gtkutil::CommandMenuItem(
            Gtk::manage(new gtkutil::IconTextMenuItem(GlobalUIManager().getLocalPixbuf(MAKE_VISPORTAL_ICON), _(MAKE_VISPORTAL))),
            "MakeVisportal",
            boost::bind(&OrthoContextMenu::checkMakeVisportal, this))
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

void OrthoContextMenu::constructMenu()
{
    _widget.reset(new Gtk::Menu);

    // Add all sections to menu

    addSectionItems(SECTION_CREATE, true); // no spacer for first category
    addSectionItems(SECTION_ACTION);
    addSectionItems(SECTION_LAYER);

    // Add the rest of the sections
    for (MenuSections::const_iterator sec = _sections.lower_bound(SECTION_LAYER+1);
         sec != _sections.end(); )
    {
        addSectionItems(sec->first);
    }

    _widget->show_all();
}

void OrthoContextMenu::addSectionItems(int section, bool noSpacer)
{
    if (_sections.find(section) == _sections.end()) return;

    const MenuItems& items = _sections[section];

    if (!noSpacer && !items.empty())
    {
        _widget->append(*Gtk::manage(new Gtk::SeparatorMenuItem));
    }

    for (MenuItems::const_iterator i = items.begin(); i != items.end(); ++i)
    {
        _widget->append(*(*i)->getWidget());
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
