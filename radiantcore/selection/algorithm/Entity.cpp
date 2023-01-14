#include "Entity.h"

#include <limits>
#include "i18n.h"
#include "itransformable.h"
#include "ieclass.h"
#include "iundo.h"
#include "selectionlib.h"
#include "iregistry.h"
#include "inamespace.h"
#include "itextstream.h"
#include "entitylib.h"
#include "gamelib.h"
#include "command/ExecutionFailure.h"
#include "command/ExecutionNotPossible.h"
#include "scene/EntitySelector.h"

#include "selection/algorithm/General.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/Group.h"
#include "eclass.h"

namespace selection 
{

namespace algorithm 
{

constexpr const char* const GKEY_BIND_KEY("/defaults/bindKey");
constexpr const char* const PLAYERSTART_CLASSNAME = "info_player_start";
// Angle key for the player start
constexpr const char* const ANGLE_KEY_NAME = "angle";
constexpr const char* const DEFAULT_ANGLE = "90"; // north

void setEntityKeyValue(const scene::INodePtr& node, const std::string& key, const std::string& value)
{
	Entity* entity = Node_getEntity(node);

	if (entity)
	{
		// Check if we have a func_static-style entity
		std::string name = entity->getKeyValue("name");
		std::string model = entity->getKeyValue("model");
		bool isFuncType = (!name.empty() && name == model);

		// Set the actual value
		entity->setKeyValue(key, value);

		// Check for name key changes of func_statics
		if (isFuncType && key == "name")
		{
			// Adapt the model key along with the name
			entity->setKeyValue("model", value);
		}
	}
	else if (Node_isPrimitive(node))
	{
		// We have a primitve node selected, check its parent
		scene::INodePtr parent(node->getParent());

		if (!parent) return;

		Entity* parentEnt = Node_getEntity(parent);

		if (parentEnt)
		{
			// We have child primitive of an entity selected, the change
			// should go right into that parent entity
			parentEnt->setKeyValue(key, value);
		}
	}
}

void setEntityClassname(const std::string& classname)
{
	if (classname.empty())
	{
		throw cmd::ExecutionFailure(_("Cannot set classname to an empty string."));
	}

	if (classname == "worldspawn")
	{
		throw cmd::ExecutionFailure(_("Cannot change classname to worldspawn."));
	}

	std::set<scene::INodePtr> entitiesToProcess;

	// Collect all entities that should have their classname set
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		// Check if we have an entity
		Entity* entity = Node_getEntity(node);

		if (entity != NULL && Node_isSelected(node))
		{
			if (!entity->isWorldspawn())
			{
				entitiesToProcess.insert(node);
			}
			else
			{
				throw cmd::ExecutionFailure(_("Cannot change classname of worldspawn entity."));
			}
		}
	});

	for (const scene::INodePtr& node : entitiesToProcess)
	{
		// "Rename" the entity, this deletes the old node and creates a new one
		scene::INodePtr newNode = changeEntityClassname(node, classname);

		// Select the new entity node
		Node_setSelected(newNode, true);
	}
}

void setEntityKeyValue(const std::string& key, const std::string& value)
{
	if (key.empty()) return;

	if (key == "name")
	{
		// Check the global namespace if this change is ok
		scene::IMapRootNodePtr mapRoot = GlobalMapModule().getRoot();

		if (mapRoot)
		{
			INamespacePtr nspace = mapRoot->getNamespace();

			if (nspace && nspace->nameExists(value))
			{
				// name exists, cancel the change
				throw cmd::ExecutionFailure(fmt::format(_("The name {0} already exists in this map!"), value));
			}
		}
	}

	// Detect classname changes
	if (key == "classname")
	{
		// Classname changes are handled in a special way
		setEntityClassname(value);
		return;
	}
	
	// Regular key change, set value on all selected entities
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		setEntityKeyValue(node, key, value);
	});
}

void setEntityKeyValueOnSelection(const cmd::ArgumentList& args)
{
	if (args.size() != 2)
	{
		rWarning() << "Usage: SetEntityKeyValue <key> <value>" << std::endl;
		return;
	}

	UndoableCommand cmd("SetEntityKeyValue");

	setEntityKeyValue(args[0].getString(), args[1].getString());
}

void bindEntities()
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == 2 && info.entityCount == 2)
	{
		UndoableCommand command("bindEntities");

		Entity* first = Node_getEntity(GlobalSelectionSystem().ultimateSelected());
		Entity* second = Node_getEntity(GlobalSelectionSystem().penultimateSelected());

		if (first != NULL && second != NULL)
		{
			// Get the bind key
			std::string bindKey = game::current::getValue<std::string>(GKEY_BIND_KEY);

			if (bindKey.empty()) {
				// Fall back to a safe default
				bindKey = "bind";
			}

			// Set the spawnarg
			second->setKeyValue(bindKey, first->getKeyValue("name"));
		}
		else
		{
			throw cmd::ExecutionFailure(_("Critical: Cannot find selected entities."));
		}
	}
	else
	{
		throw cmd::ExecutionNotPossible(
			_("Exactly two entities must be selected for this operation."));
	}
}

void connectSelectedEntities()
{
	if (GlobalSelectionSystem().countSelected() == 2)
	{
		// Obtain both entities
		Entity* e1 = Node_getEntity(GlobalSelectionSystem().penultimateSelected()); // source
		Entity* e2 = Node_getEntity(GlobalSelectionSystem().ultimateSelected()); // target

		// Check entities are valid
		if (e1 == nullptr || e2 == nullptr)
		{
			rError() << "connectSelectedEntities: both of the selected instances must be entities" << std::endl;
			return;
		}

		// Check entities are distinct
		if (e1 == e2) 
		{
			rError() << "connectSelectedEntities: the selected entities must be different" << std::endl;
			return;
		}

		// Start the scoped undo session
		UndoableCommand undo("entityConnectSelected");

		// Find the first unused target key on the source entity
		for (unsigned int i = 0; i < std::numeric_limits<unsigned int>::max(); ++i)
		{
			// Construct candidate key by appending number to "target"
			auto targetKey = fmt::format("target{0:d}", i);

			// If the source entity does not have this key, add it and finish,
			// otherwise continue looping
			if (e1->getKeyValue(targetKey).empty())
			{
				e1->setKeyValue(targetKey, e2->getKeyValue("name"));
				break;
			}
		}

		// Redraw the scene
		SceneChangeNotify();
	}
	else
	{
		throw cmd::ExecutionNotPossible(
			_("Exactly two entities must be selected for this operation."));
	}
}

bool entityReferencesModel(const Entity& entity, const std::string& searchString)
{
    auto model = entity.getKeyValue("model");

    if (searchString == model)
    {
        return true;
    }

    // The entity might still reference the model through a model def
    auto modelDef = GlobalEntityClassManager().findModel(model);

    return modelDef && modelDef->getMesh() == searchString;
}

void selectItemsByModel(const std::string& model)
{
    if (!GlobalSceneGraph().root()) return;

    scene::EntitySelector selector([&](const Entity& entity)
    {
        return entityReferencesModel(entity, model);
    }, true);

    GlobalSceneGraph().root()->traverse(selector);
}

void deselectItemsByModel(const std::string& model)
{
    if (!GlobalSceneGraph().root()) return;
    
    scene::EntitySelector deselector([&](const Entity& entity)
    {
        return entityReferencesModel(entity, model);
    }, false);

    GlobalSceneGraph().root()->traverse(deselector);
}

// Command target to (de-)select items by model
void selectItemsByModelCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rMessage() << "Usage: SelectItemsByModel <modelpath>" << std::endl;
        return;
    }

    selectItemsByModel(args[0].getString());
}

void deselectItemsByModelCmd(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rMessage() << "Usage: DeselectItemsByModel <modelpath>" << std::endl;
        return;
    }

    deselectItemsByModel(args[0].getString());
}

void placePlayerStart(const cmd::ArgumentList& args)
{
    if (args.size() != 1)
    {
        rWarning() << "Usage: PlacePlayerStart <position:vector3>" << std::endl;
        return;
    }

    auto position = args[0].getVector3();

    UndoableCommand command(_("Place Player Start"));

    EntityNodeFindByClassnameWalker walker(PLAYERSTART_CLASSNAME);
    GlobalSceneGraph().root()->traverse(walker);

    auto playerStartNode = walker.getEntityNode();
    auto playerStartEntity = walker.getEntity();

    if (playerStartEntity == nullptr)
    {
        // Create the player start entity
        auto eclass = GlobalEntityClassManager().findClass(PLAYERSTART_CLASSNAME);
        if (!eclass) throw cmd::ExecutionNotPossible(_("Could not find the info_player_start entityDef"));

        playerStartNode = GlobalEntityModule().createEntity(eclass);
        scene::addNodeToContainer(playerStartNode, GlobalSceneGraph().root());

        playerStartEntity = Node_getEntity(playerStartNode);

        // Set a default angle
        playerStartEntity->setKeyValue(ANGLE_KEY_NAME, DEFAULT_ANGLE);
    }

    playerStartEntity->setKeyValue("origin", string::to_string(position));

    // #5972: Leave player start selected after placement
    Node_setSelected(playerStartNode, true);
}

} // namespace algorithm
} // namespace selection
