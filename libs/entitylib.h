#pragma once

#include "ientity.h"
#include "ieclass.h"
#include "iselection.h"
#include "iselectiontest.h"

#include "math/AABB.h"
#include "scenelib.h"

#include "imd5anim.h"
#include "imd5model.h"
#include "imodel.h"

class SelectionIntersection;

inline void aabb_testselect(const AABB& aabb, SelectionTest& test, SelectionIntersection& best)
{
  const IndexPointer::index_type indices[24] = {
    2, 1, 5, 6,
    1, 0, 4, 5,
    0, 1, 2, 3,
    3, 7, 4, 0,
    3, 2, 6, 7,
    7, 6, 5, 4,
  };

  Vector3 points[8];
  aabb.getCorners(points);
  VertexPointer pointer(points, sizeof(Vector3));
  test.TestQuads(pointer, IndexPointer(indices, 24), best);
}

/**
 * Stream insertion for Entity objects.
 */
inline std::ostream& operator<< (std::ostream& os, const Entity& entity) {
	os << "Entity { name=\"" << entity.getKeyValue("name") << "\", "
	   << "classname=\"" << entity.getKeyValue("classname") << "\", "
	   << "origin=\"" << entity.getKeyValue("origin") << "\" }";

	return os;
}

class EntityNodeFindByClassnameWalker :
	public scene::NodeVisitor
{
protected:
	// Name to search for
	std::string _name;

	// The search result
	scene::INodePtr _entityNode;

public:
	// Constructor
	EntityNodeFindByClassnameWalker(const std::string& name) :
		_name(name)
	{}

	scene::INodePtr getEntityNode() {
		return _entityNode;
	}

	Entity* getEntity() {
		return _entityNode != NULL ? Node_getEntity(_entityNode) : NULL;
	}

	// Pre-descent callback
	bool pre(const scene::INodePtr& node) {
		if (_entityNode == NULL) {
			// Entity not found yet
			Entity* entity = Node_getEntity(node);

			if (entity != NULL) {
				// Got an entity, let's see if the name matches
				if (entity->getKeyValue("classname") == _name) {
					_entityNode = node;
				}

				return false; // don't traverse entities
			}
			else {
				// Not an entity, traverse
				return true;
			}
		}
		else {
			// Entity already found, don't traverse any further
			return false;
		}
	}
};

/* greebo: Finds an entity with the given classname
 */
inline Entity* Scene_FindEntityByClass(const std::string& className)
{
	// Instantiate a walker to find the entity
	EntityNodeFindByClassnameWalker walker(className);

	// Walk the scenegraph
	GlobalSceneGraph().root()->traverse(walker);

	return walker.getEntity();
}

/* Check if a node is the worldspawn.
 */
inline bool Node_isWorldspawn(const scene::INodePtr& node) 
{
	Entity* entity = Node_getEntity(node);

	return entity != nullptr && entity->isWorldspawn();
}

/**
 * greebo: Changing the entity classname is a non-trivial operation in DarkRadiant, as
 * the actual c++ class of an entity is depending on it. Changing the classname
 * therefore means 1) to recreate a new entity 2) to copy all spawnargs over from the old one
 * and 3) re-parent any child nodes to the new entity.
 *
 * @node: The entity node to change the classname of.
 * @classname: The new classname.
 *
 * @returns: The new entity node.
 */
inline scene::INodePtr changeEntityClassname(const scene::INodePtr& node,
                                             const std::string& classname)
{
	// Make a copy of this node first
	scene::INodePtr oldNode(node);

	// greebo: First, get the eclass
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(
		classname,
		scene::hasChildPrimitives(oldNode) // whether this entity has child primitives
	);

	// must not fail, findOrInsert always returns non-NULL
	assert(eclass);

	// Create a new entity with the given class
	IEntityNodePtr newNode(GlobalEntityModule().createEntity(eclass));

	Entity* oldEntity = Node_getEntity(oldNode);

	// Traverse the old entity with a walker
	Entity& newEntity = newNode->getEntity();

    // Copy all keyvalues except classname
    oldEntity->forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        if (key != "classname") 
        {
            newEntity.setKeyValue(key, value);
        }
    });

	// Remember the oldNode's parent before removing it
	scene::INodePtr parent = oldNode->getParent();

	// The old node must not be the root node or an orphaned one
	assert(parent);

	// Traverse the child and reparent all primitives to the new entity node
	scene::parentPrimitives(oldNode, newNode);

	// Remove the old entity node from the parent. This will disconnect 
	// oldNode from the scene and the UndoSystem, so it's important to do 
	// this step last, after the primitives have been moved. (#4718)
	scene::removeNodeFromParent(oldNode);

	// Let the new node keep its layer information (#4710)
    // Apply the layers to the whole subgraph (#5214)
    scene::AssignNodeToLayersWalker layerWalker(oldNode->getLayers());
    newNode->traverse(layerWalker);

	// Insert the new entity to the parent
	parent->addChildNode(newNode);

	return newNode;
}

/**
 * greebo: This class can be used to traverse a subgraph to search
 * for a specific spawnarg on the worldspawn entity. The method
 * getValue() can be used to retrieve the value of the key as specified
 * in the constructor.
 */
class WorldspawnArgFinder :
    public scene::NodeVisitor
{
    std::string _key;
    std::string _value;

public:
    WorldspawnArgFinder(const std::string& keyName) :
        _key(keyName)
    {}

    bool pre(const scene::INodePtr& node) override
    {
        // Try to cast this node onto an entity
        auto* ent = Node_getEntity(node);

        if (ent != nullptr)
        {
            if (ent->isWorldspawn())
            {
                // Load the description spawnarg
                _value = ent->getKeyValue(_key);
            }

            return false; // don't traverse entities
        }

        return true;
    }

    /**
     * Returns the found value for the desired spawnarg. If not found,
     * this function will return an empty string "".
     */
    const std::string& getFoundValue() const
    {
        return _value;
    }
};

namespace scene
{

/**
 * Apply the idle anim to the given scene node using the given modelDef
 */
inline void applyIdlePose(const INodePtr& node, const IModelDef::Ptr& modelDef)
{
    auto modelNode = Node_getModel(node);

    if (!modelNode) return;

    // Set the animation to play
    auto md5model = dynamic_cast<md5::IMD5Model*>(&(modelNode->getIModel()));

    if (!md5model) return;

    // Look up the "idle" anim if there is one
    auto found = modelDef->getAnim("idle");

    if (found.empty()) return;

    // Load the anim
    if (auto anim = GlobalAnimationCache().getAnim(found))
    {
        md5model->setAnim(anim);
        md5model->updateAnim(0);
    }
}

inline void foreachSelectedEntity(const std::function<void(Entity&)>& functor)
{
    GlobalSelectionSystem().foreachSelected([&](const INodePtr& node)
    {
        if (Node_isEntity(node))
        {
            functor(*Node_getEntity(node));
        }
    });
}

}
