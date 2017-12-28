#include "Entity.h"

#include "i18n.h"
#include "itransformable.h"
#include "selectionlib.h"
#include "imainframe.h"
#include "iregistry.h"
#include "itextstream.h"
#include "entitylib.h"
#include "gamelib.h"
#include "wxutil/dialog/MessageBox.h"

#include "selection/algorithm/General.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/Group.h"
#include "map/Map.h"
#include "eclass.h"

namespace selection 
{

namespace algorithm 
{

const char* const GKEY_BIND_KEY("/defaults/bindKey");

void setEntityKeyvalue(const scene::INodePtr& node, const std::string& key, const std::string& value)
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

void setEntityKeyvalue(const std::string& key, const std::string& value)
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
				throw std::runtime_error(fmt::format(_("The name {0} already exists in this map!"), value));
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
		setEntityKeyvalue(node, key, value);
	});
}

void setEntityClassname(const std::string& classname) 
{
	if (classname.empty())
	{
		throw std::runtime_error(_("Cannot set classname to an empty string."));
	}

	if (classname == "worldspawn")
	{
		throw std::runtime_error(_("Cannot change classname to worldspawn."));
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
				throw std::runtime_error(_("Cannot change classname of worldspawn entity."));
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

void bindEntities(const cmd::ArgumentList& args) {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == 2 && info.entityCount == 2) {
		UndoableCommand command("bindEntities");

		Entity* first = Node_getEntity(GlobalSelectionSystem().ultimateSelected());
		Entity* second = Node_getEntity(GlobalSelectionSystem().penultimateSelected());

		if (first != NULL && second != NULL) {
			// Get the bind key
			std::string bindKey = game::current::getValue<std::string>(GKEY_BIND_KEY);

			if (bindKey.empty()) {
				// Fall back to a safe default
				bindKey = "bind";
			}

			// Set the spawnarg
			second->setKeyValue(bindKey, first->getKeyValue("name"));
		}
		else {
			wxutil::Messagebox::ShowError(
				_("Critical: Cannot find selected entities."));
		}
	}
	else {
		wxutil::Messagebox::ShowError(
			_("Exactly two entities must be selected for this operation."));
	}
}

void connectSelectedEntities(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 2)
	{
		GlobalEntityCreator().connectEntities(
			GlobalSelectionSystem().penultimateSelected(),	// source
			GlobalSelectionSystem().ultimateSelected()		// target
		);
	}
	else
	{
		wxutil::Messagebox::ShowError(
			_("Exactly two entities must be selected for this operation."));
	}
}

// Default radius of lights is 320 (Q4) rather than 300 (D3)
// since the grid is usually a multiple of 8.
const float DEFAULT_LIGHT_RADIUS = 320;

// Function to return an AABB based on the current workzone AABB (retrieved
// from the currently selected brushes), or to use the default light radius
// if the workzone AABB is not valid or none is available.
AABB Doom3Light_getBounds(AABB aabb)
{
    // If the extents are 0 or invalid (-1), replace with the default radius
    for (int i = 0; i < 3; i++)
	{
        if (aabb.extents[i] <= 0)
		{
            aabb.extents[i] = DEFAULT_LIGHT_RADIUS;
		}
    }

    return aabb;
}

/**
 * Create an instance of the given entity at the given position, and return
 * the Node containing the new entity.
 *
 * @returns: the scene::INodePtr referring to the new entity.
 */
scene::INodePtr createEntityFromSelection(const std::string& name, const Vector3& origin)
{
    // Obtain the structure containing the selection counts
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    IEntityClassPtr entityClass = GlobalEntityClassManager().findOrInsert(name, true);

    // TODO: to be replaced by inheritance-based class detection
    bool isModel = (info.totalCount == 0 && name == "func_static");

    // Some entities are based on the size of the currently-selected primitive(s)
    bool primitivesSelected = info.brushCount > 0 || info.patchCount > 0;

    if (!(entityClass->isFixedSize() || isModel) && !primitivesSelected) {
        throw EntityCreationException(
            fmt::format(_("Unable to create entity {0}, no brushes selected."), name)
        );
    }

    // Get the selection workzone bounds
    AABB workzone = GlobalSelectionSystem().getWorkZone().bounds;

    // Create the new node for the entity
    IEntityNodePtr node(GlobalEntityCreator().createEntity(entityClass));

    GlobalSceneGraph().root()->addChildNode(node);

    // The layer list we're moving the newly created node/subgraph into
    scene::LayerList targetLayers;

    if (entityClass->isFixedSize() || (isModel && !primitivesSelected))
    {
        selection::algorithm::deleteSelection();

        ITransformablePtr transform = Node_getTransformable(node);

        if (transform != 0) {
            transform->setType(TRANSFORM_PRIMITIVE);
            transform->setTranslation(origin);
            transform->freezeTransform();
        }

        GlobalSelectionSystem().setSelectedAll(false);

        // Move the item to the active layer
        targetLayers.insert(GlobalLayerSystem().getActiveLayer());

        Node_setSelected(node, true);
    }
    else // brush-based entity
    {
        // Add selected brushes as children of non-fixed entity
        node->getEntity().setKeyValue("model",
                                      node->getEntity().getKeyValue("name"));

        // Take the selection center as new origin
        Vector3 newOrigin = selection::algorithm::getCurrentSelectionCenter();
        node->getEntity().setKeyValue("origin", string::to_string(newOrigin));

        // If there is an "editor_material" class attribute, apply this shader
        // to all of the selected primitives before parenting them
        std::string material = node->getEntity().getEntityClass()->getAttribute("editor_material").getValue();

        if (!material.empty())
		{
            selection::algorithm::applyShaderToSelection(material);
        }

        // If we had primitives to reparent, the new entity should inherit the layer info from them
        if (primitivesSelected)
        {
            scene::INodePtr primitive = GlobalSelectionSystem().ultimateSelected();
            targetLayers = primitive->getLayers();
        }
        else
        {
            // Otherwise move the item to the active layer
            targetLayers.insert(GlobalLayerSystem().getActiveLayer());
        }

        // Parent the selected primitives to the new node
        selection::algorithm::ParentPrimitivesToEntityWalker walker(node);
        GlobalSelectionSystem().foreachSelected(walker);
        walker.reparent();

        // De-select the children and select the newly created parent entity
        GlobalSelectionSystem().setSelectedAll(false);
        Node_setSelected(node, true);
    }

    // Assign the layers - including all child nodes (#2864)
    scene::AssignNodeToLayersWalker layerWalker(targetLayers);
    node->traverse(layerWalker);

    // Set the light radius and origin

    if (entityClass->isLight() && primitivesSelected)
    {
        AABB bounds(Doom3Light_getBounds(workzone));
        node->getEntity().setKeyValue("origin",
                                      string::to_string(bounds.getOrigin()));
        node->getEntity().setKeyValue("light_radius",
                                      string::to_string(bounds.getExtents()));
    }

    // Flag the map as unsaved after creating the entity
    GlobalMap().setModified(true);

    // Check for auto-setting key values. TODO: use forEachClassAttribute
    // directly here.
    eclass::AttributeList list = eclass::getSpawnargsWithPrefix(
        *entityClass, "editor_setKeyValue"
    );

    if (!list.empty())
    {
        for (eclass::AttributeList::const_iterator i = list.begin(); i != list.end(); ++i)
        {
            // Cut off the "editor_setKeyValueN " string from the key to get the spawnarg name
            std::string key = i->getName().substr(i->getName().find_first_of(' ') + 1);
            node->getEntity().setKeyValue(key, i->getValue());
        }
    }

    // Return the new node
    return node;
}

} // namespace algorithm
} // namespace selection
