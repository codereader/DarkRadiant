#include "Entity.h"

#include "i18n.h"
#include "itransformable.h"
#include "selectionlib.h"
#include "imainframe.h"
#include "iregistry.h"
#include "itextstream.h"
#include "entitylib.h"
#include "gamelib.h"
#include "gtkutil/dialog/MessageBox.h"

#include "selection/algorithm/General.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/Group.h"
#include "map/Map.h"
#include "eclass.h"

namespace selection {
	namespace algorithm {

const char* const GKEY_BIND_KEY("/defaults/bindKey");

/**
 * greebo: This walker traverses a subgraph and changes the classname
 *         of all selected entities to the one passed to the constructor.
 */
class EntitySetClassnameSelected :
	public SelectionSystem::Visitor
{
	std::string _classname;

	// Entites are getting accumulated in this list and processed at destruction time
	mutable std::set<scene::INodePtr> _entities;
public:
	EntitySetClassnameSelected(const std::string& classname) :
		_classname(classname)
	{}

	~EntitySetClassnameSelected() {
		for (std::set<scene::INodePtr>::iterator i = _entities.begin();
			 i != _entities.end(); ++i)
		{
			// "Rename" the entity, this deletes the old node and creates a new one
			scene::INodePtr newNode = changeEntityClassname(*i, _classname);

			// Select the new entity node
			Node_setSelected(newNode, true);
		}
	}

	virtual void visit(const scene::INodePtr& node) const {
		// Check if we have an entity
		Entity* entity = Node_getEntity(node);

		if (entity != NULL && Node_isSelected(node)) {
			if (entity->getKeyValue("classname") != "worldspawn") {
				_entities.insert(node);
			}
			else {
				gtkutil::MessageBox::ShowError(
					_("Cannot change classname of worldspawn entity."),
					GlobalMainFrame().getTopLevelWindow());
			}
		}
	}
};

void setEntityClassname(const std::string& classname) {

	if (classname.empty()) {
		rError() << "Cannot set classname to an empty string!" << std::endl;
	}

	// greebo: instantiate a walker and traverse the current selection
	EntitySetClassnameSelected classnameSetter(classname);
	GlobalSelectionSystem().foreachSelected(classnameSetter);

	// The destructor of the classNameSetter will rename the entities
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
			gtkutil::MessageBox::ShowError(
				_("Critical: Cannot find selected entities."),
				GlobalMainFrame().getTopLevelWindow());
		}
	}
	else {
		gtkutil::MessageBox::ShowError(
			_("Exactly two entities must be selected for this operation."),
			GlobalMainFrame().getTopLevelWindow());
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
		gtkutil::MessageBox::ShowError(
			_("Exactly two entities must be selected for this operation."),
			GlobalMainFrame().getTopLevelWindow());
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
            (boost::format(_("Unable to create entity %s, no brushes selected.")) % name).str()
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
