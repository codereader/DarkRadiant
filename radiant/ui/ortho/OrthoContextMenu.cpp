#include "OrthoContextMenu.h"

#include "i18n.h"
#include "selectionlib.h"
#include "isound.h"
#include "ui/iresourcechooser.h"
#include "ui/idialogmanager.h"
#include "entitylib.h" // EntityFindByClassnameWalker
#include "ientity.h" // Node_getEntity()
#include "iregistry.h"
#include "ui/imainframe.h"

#include "wxutil/dialog/MessageBox.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/menu/CommandMenuItem.h"
#include "wxutil/EntityClassChooser.h"

#include "ui/modelselector/ModelSelector.h"
#include "ui/prefabselector/PrefabSelector.h"
#include "ui/particles/ParticleChooserDialog.h"

#include "string/convert.h"
#include "scene/GroupNodeChecker.h"
#include "scene/ModelFinder.h"
#include "math/AABB.h"

#include <wx/window.h>
#include <wx/menu.h>

#include "module/StaticModule.h"
#include "command/ExecutionFailure.h"

namespace ui
{

namespace {

    /* CONSTANTS */

    const char* LIGHT_CLASSNAME = "light";
    const char* MODEL_CLASSNAME_ANIMATED = "func_animate";
    const char* MODEL_CLASSNAME_STATIC = "func_static";
    const char* PLAYERSTART_CLASSNAME = "info_player_start";

    const char* ADD_ENTITY_TEXT = N_("Create Entity...");
    const char* CONVERT_TO_ENTITY_TEXT = N_("Convert to Entity...");
    const char* ADD_ENTITY_ICON = "cmenu_add_entity.png";
    const char* PLACE_PLAYERSTART_TEXT = N_("Place Player Start here");
    const char* PLACE_PLAYERSTART_ICON = "player_start16.png";
    const char* ADD_MODEL_TEXT = N_("Create Model...");
    const char* ADD_MODEL_ICON = "cmenu_add_model.png";
    const char* ADD_PARTICLE_TEXT = N_("Create Particle...");
    const char* ADD_PARTICLE_ICON = "particle16.png";
    const char* ADD_MONSTERCLIP_TEXT = N_("Surround with Monsterclip");
    const char* ADD_MONSTERCLIP_ICON = "monsterclip16.png";
    const char* ADD_LIGHT_TEXT = N_("Create Light...");
    const char* ADD_LIGHT_ICON = "cmenu_add_light.png";
    const char* ADD_PREFAB_TEXT = N_("Insert Prefab...");
    const char* ADD_PREFAB_ICON = "cmenu_add_prefab.png";
    const char* ADD_SPEAKER_TEXT = N_("Create Speaker...");
    const char* ADD_SPEAKER_ICON = "icon_sound.png";
    const char* CONVERT_TO_STATIC_TEXT = N_("Convert to func_static");
    const char* CONVERT_TO_STATIC_ICON = "cmenu_convert_static.png";
    const char* REPARENT_PRIMITIVES_TEXT = N_("Reparent Primitives");
    const char* REVERT_TO_WORLDSPAWN_TEXT = N_("Revert to Worldspawn");
    const char* REVERT_TO_WORLDSPAWN_PARTIAL_TEXT = N_("Revert Part to Worldspawn");
    const char* REVERT_TO_WORLDSPAWN_ICON = "cmenu_revert_worldspawn.png";
    const char* MERGE_ENTITIES_ICON = "cmenu_merge_entities.png";
    const char* MERGE_ENTITIES_TEXT = N_("Merge Entities");
    const char* MAKE_VISPORTAL = N_("Make Visportal");
    const char* MAKE_VISPORTAL_ICON = "make_visportal.png";
}

// Define the static OrthoContextMenu module
module::StaticModuleRegistration<OrthoContextMenu> orthoContextMenuModule;

OrthoContextMenu& OrthoContextMenu::Instance()
{
    return *std::static_pointer_cast<OrthoContextMenu>(
        module::GlobalModuleRegistry().getModule("OrthoContextMenu"));
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
    for (const auto& [_, section] : _sections)
    {
        for (const auto& item : section)
        {
            bool visible = item->isVisible();

            if (visible)
            {
                // Visibility check passed
                // Run the preshow command
                item->preShow();

                item->getMenuItem()->Enable(item->isSensitive());
            }
            else
            {
                // Visibility check failed, skip sensitivity check
                item->getMenuItem()->Enable(false);
            }
        }
    }

    // Adjust the Create Entity label, it's Convert to Entity if anything is selected
    _createEntityItem->getMenuItem()->SetItemLabel(
        _selectionInfo.anythingSelected ? CONVERT_TO_ENTITY_TEXT : ADD_ENTITY_TEXT);

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
        scene::GroupNodeChecker walker;
        GlobalSelectionSystem().foreachSelected(walker);

        _selectionInfo.onlyGroupsSelected = walker.onlyGroupsAreSelected();
        _selectionInfo.singleGroupSelected = walker.selectedGroupCount() == 1 && !Node_isWorldspawn(walker.getFirstSelectedGroupNode());

        // Create a ModelFinder and check whether only models were selected
        scene::ModelFinder visitor;
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

bool OrthoContextMenu::checkAddModelOrParticle()
{
    return !_selectionInfo.anythingSelected;
}

bool OrthoContextMenu::checkPlacePlayerStart()
{
    return !_selectionInfo.anythingSelected;
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
    return selection::curSelectionIsSuitableForReparent();
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
    // Display the chooser to select an entity classname
    auto purpose = _selectionInfo.anythingSelected ?
        wxutil::EntityClassChooser::Purpose::ConvertEntity :
        wxutil::EntityClassChooser::Purpose::AddEntity;

    auto className = wxutil::EntityClassChooser::ChooseEntityClass(purpose);

    if (className.empty()) return;

    UndoableCommand command(_selectionInfo.anythingSelected ? "convertToEntity" : "createEntity");

    // Create the entity. We might get an EntityCreationException if the
    // wrong number of brushes is selected.
    try
    {
        GlobalEntityModule().createEntityFromSelection(className, _lastPoint);
    }
    catch (cmd::ExecutionFailure& e)
    {
        wxutil::Messagebox::ShowError(e.what());
    }
}

void OrthoContextMenu::placePlayerStart()
{
    GlobalCommandSystem().executeCommand("PlacePlayerStart", cmd::Argument(_lastPoint));
}

void OrthoContextMenu::callbackAddLight()
{
    UndoableCommand command("addLight");

    try
	{
        GlobalEntityModule().createEntityFromSelection(LIGHT_CLASSNAME, _lastPoint);
    }
    catch (cmd::ExecutionFailure& e)
	{
        wxutil::Messagebox::ShowError(fmt::format(_("Unable to create light: {0}"), e.what()));
    }
}

void OrthoContextMenu::callbackAddPrefab()
{
    auto result = PrefabSelector::ChoosePrefab();

    if (!result.prefabPath.empty())
    {
        // Pass the call to the map algorithm and give the lastPoint coordinate as argument
        GlobalCommandSystem().executeCommand(LOAD_PREFAB_AT_CMD,
            result.prefabPath, _lastPoint, result.insertAsGroup);
    }
}

void OrthoContextMenu::callbackAddSpeaker()
{
    // If we have an active sound module, query the desired shader from the user
    if (!module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
    {
        return;
    }

    IResourceChooser* chooser = GlobalDialogManager().createSoundShaderChooser();

    // Use a SoundChooser dialog to get a selection from the user
    std::string shaderPath = chooser->chooseResource();

    chooser->destroyDialog();

    if (shaderPath.empty())
    {
        return; // user cancelled the dialog, don't do anything
    }

    GlobalCommandSystem().executeCommand("CreateSpeaker", {
        cmd::Argument(shaderPath), cmd::Argument(_lastPoint)
    });
}

void OrthoContextMenu::callbackAddModel()
{
    UndoableCommand command("addModel");

    // Display the model selector and block waiting for a selection (may be empty)
    ModelSelectorResult ms = ModelSelector::chooseModel("", true, true);

    // If a model was selected, create the entity and set its model key
    if (ms.model.empty())
    {
        return;
    }

    try
    {
        auto modelDef = GlobalEntityClassManager().findModel(ms.model);

        auto className = modelDef ? MODEL_CLASSNAME_ANIMATED : MODEL_CLASSNAME_STATIC;

        auto modelNode = GlobalEntityModule().createEntityFromSelection(
            className, _lastPoint
        );

        //Node_getTraversable(GlobalSceneGraph().root())->insert(modelNode);
        modelNode->getEntity().setKeyValue("model", ms.model);
        modelNode->getEntity().setKeyValue("skin", ms.skin);

        // If 'createClip' is ticked, create a clip brush
        if (ms.createClip)
        {
            GlobalCommandSystem().execute("SurroundWithMonsterclip");
        }
    }
    catch (cmd::ExecutionFailure& e)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Unable to create model: {0}"), e.what()));
    }
}

void OrthoContextMenu::callbackAddParticle()
{
    UndoableCommand command("Create Particle");

    // Display the particle selector and block waiting for a selection (may be empty)
    auto result = ParticleChooserDialog::ChooseParticleAndEmitter();

    if (result.selectedParticle.empty()) return;

    try
    {
        auto node = GlobalEntityModule().createEntityFromSelection(result.selectedClassname, _lastPoint);

        auto modelSpawnarg = result.selectedClassname == "func_smoke" ? "smoke" : "model";
        node->getEntity().setKeyValue(modelSpawnarg, result.selectedParticle);
    }
    catch (cmd::ExecutionFailure& e)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Unable to create particle entity: {0}"), e.what()));
    }
}

void OrthoContextMenu::registerDefaultItems()
{
    _createEntityItem = std::make_shared<wxutil::MenuItem>(
        new wxutil::IconTextMenuItem(_(ADD_ENTITY_TEXT), ADD_ENTITY_ICON),
        std::bind(&OrthoContextMenu::addEntity, this),
        std::bind(&OrthoContextMenu::checkAddEntity, this));

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
            std::bind(&OrthoContextMenu::checkAddModelOrParticle, this))
    );

    wxutil::MenuItemPtr addParticle(
        new wxutil::MenuItem(
            new wxutil::IconTextMenuItem(_(ADD_PARTICLE_TEXT), ADD_PARTICLE_ICON),
            std::bind(&OrthoContextMenu::callbackAddParticle, this),
            std::bind(&OrthoContextMenu::checkAddModelOrParticle, this))
    );

    wxutil::CommandMenuItemPtr surroundWithMonsterClip(
        new wxutil::CommandMenuItem(
			new wxutil::IconTextMenuItem(_(ADD_MONSTERCLIP_TEXT), ADD_MONSTERCLIP_ICON),
            "SurroundWithMonsterclip",
            std::bind(&OrthoContextMenu::checkAddMonsterclip, this),
            std::bind(&OrthoContextMenu::checkAddMonsterclip, this))
    );

    wxutil::MenuItemPtr placePlayerStart(
        new wxutil::MenuItem(
			new wxutil::IconTextMenuItem(_(PLACE_PLAYERSTART_TEXT), PLACE_PLAYERSTART_ICON),
            std::bind(&OrthoContextMenu::placePlayerStart, this),
            std::bind(&OrthoContextMenu::checkPlacePlayerStart, this),
            std::bind(&OrthoContextMenu::checkPlacePlayerStart, this))
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
    addItem(_createEntityItem, SECTION_CREATE);
    addItem(addModel, SECTION_CREATE);
    addItem(addParticle, SECTION_CREATE);
    addItem(addLight, SECTION_CREATE);
    addItem(addSpeaker, SECTION_CREATE);
    addItem(addPrefab, SECTION_CREATE);

    addItem(placePlayerStart, SECTION_ACTION);
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
	addSectionItems(SECTION_FILTER);
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
    static std::string _name(MODULE_ORTHOCONTEXTMENU);
    return _name;
}

const StringSet& OrthoContextMenu::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_COMMANDSYSTEM,
        MODULE_MAINFRAME,
    };

    return _dependencies;
}

void OrthoContextMenu::initialiseModule(const IApplicationContext& ctx)
{
    GlobalMainFrame().signal_MainFrameConstructed().connect(
        sigc::mem_fun(this, &OrthoContextMenu::constructMenu)
    );

    registerDefaultItems();
}

void OrthoContextMenu::shutdownModule()
{
    _createEntityItem.reset();
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
