#pragma once

#include "inode.h"
#include "ilayer.h"
#include "iscenegraph.h"
#include "iselectable.h"
#include "ipatch.h"
#include "ibrush.h"
#include "icameraview.h"

#include "scene/Node.h"

#include <stack>

inline bool Node_isPrimitive(const scene::INodePtr& node)
{
    scene::INode::Type type = node->getNodeType();
	// greebo: Changed this routine to use the nodeType enum instead of two dynamic casts
	// There shouldn't be any discrepancies, but I'll leave this assertion in here for a while
    assert((type == scene::INode::Type::Brush || type == scene::INode::Type::Patch) == (Node_isBrush(node) || Node_isPatch(node)));

    return type == scene::INode::Type::Brush || type == scene::INode::Type::Patch;
}

namespace scene
{

// Reparents every visited primitive to the parent in the constructor arguments
class PrimitiveReparentor :
    public scene::NodeVisitor
{
private:
    scene::INodePtr _parent;

public:
    PrimitiveReparentor(const scene::INodePtr& parent) :
        _parent(parent)
    {}

    virtual bool pre(const scene::INodePtr& node) override
    {
        return false;
    }

    virtual void post(const scene::INodePtr& node) override
    {
        if (!Node_isPrimitive(node))
        {
            return;
        }

        // We need to keep the hard reference to the node, such that the refcount doesn't reach 0
        scene::INodePtr nodeRef = node;

        scene::INodePtr oldParent = nodeRef->getParent();

        if (oldParent)
        {
            // greebo: remove the node from the old parent first
            oldParent->removeChildNode(nodeRef);
        }

        _parent->addChildNode(nodeRef);
    }
};

inline void parentPrimitives(const scene::INodePtr& subgraph, const scene::INodePtr& parent)
{
    PrimitiveReparentor visitor(parent);
    subgraph->traverseChildren(visitor);
}

/**
 * Returns true if the given node contains
 * child primitives. Being an entity is not enough.
 */
inline bool hasChildPrimitives(const INodePtr& node)
{
    // A node without child nodes is not a group
    if (!node->hasChildNodes())
	{
        return false;
    }

	bool hasPrimitives = false;

	node->foreachNode([&] (const INodePtr& child)->bool
	{
		if (Node_isPrimitive(child))
		{
            hasPrimitives = true;
			return false; // don't traverse any further
        }
		else
		{
			return true;
		}
	});

    return hasPrimitives;
}

/**
 * greebo: This removes the given node from its parent node.
 *         The node is also deselected beforehand.
 */
inline void removeNodeFromParent(const INodePtr& node)
{
    // Check if the node has a parent in the first place
    INodePtr parent = node->getParent();

    if (parent != NULL)
	{
        // Unselect the node
        Node_setSelected(node, false);

        parent->removeChildNode(node);
    }
}

/**
 * This assigns every visited node to the given set of layers.
 * Any previous assignments of the node get overwritten by this routine.
 */
class AssignNodeToLayersWalker :
    public NodeVisitor
{
    const LayerList& _layers;
public:
    AssignNodeToLayersWalker(const LayerList& layers) :
        _layers(layers)
    {}

    bool pre(const INodePtr& node)
	{
        // Pass the call to the single-node method
		node->assignToLayers(_layers);

        return true; // full traverse
    }
};

class UpdateNodeVisibilityWalker :
    public NodeVisitor
{
    std::stack<bool> _visibilityStack;

    scene::IMapRootNodePtr _root;
public:
    UpdateNodeVisibilityWalker(const scene::IMapRootNodePtr& root) :
        _root(root)
    {
        assert(_root);
    }

    bool pre(const INodePtr& node) 
    {
        // Update the node visibility and store the result
        bool nodeIsVisible = _root->getLayerManager().updateNodeVisibility(node);

        // Add a new element for this level
        _visibilityStack.push(nodeIsVisible);

        return true;
    }

    void post(const INodePtr& node) 
    {
        // Is this child visible?
        bool childIsVisible = _visibilityStack.top();

        _visibilityStack.pop();

        if (childIsVisible) 
        {
            // Show the node, regardless whether it was hidden before
            // otherwise the parent would hide the visible children as well
            node->disable(Node::eLayered);
        }

        if (node->checkStateFlag(Node::eLayered))
		{
            // Node is hidden by layers after update (and no children are visible), de-select
            Node_setSelected(node, false);
        }

        if (childIsVisible && !_visibilityStack.empty()) 
        {
            // The child was visible, set this parent to true
            _visibilityStack.top() = true;
        }
    }
};

/**
 * greebo: This method inserts the given node into the given container
 *         and ensures that the container's layer visibility is updated.
 */
inline void addNodeToContainer(const INodePtr& node, const INodePtr& container) 
{
    // Insert the child
    container->addChildNode(node);

    // If the container is already connected to a root, check the layer visibility
    auto rootNode = container->getRootNode();

    if (rootNode)
    {
        // Ensure that worldspawn is visible
        UpdateNodeVisibilityWalker walker(rootNode);
        container->traverse(walker);
    }
}

} // namespace scene

inline bool Node_hasSelectedChildNodes(const scene::INodePtr& node)
{
    bool selected = false;

	node->foreachNode([&] (const scene::INodePtr& child)->bool
	{
		if (Node_isSelected(child))
		{
            selected = true;
			return false; // stop searching
        }

        return true;
	});

    return selected;
}

namespace scene
{

inline void setNodeHidden(const INodePtr& node, bool hide)
{
    if (!node->supportsStateFlag(Node::eHidden))
    {
        return;
    }

    if (hide)
    {
        node->enable(Node::eHidden);
    }
    else
    {
        node->disable(Node::eHidden);
    }
}

inline void showNode(const INodePtr& node)
{
    setNodeHidden(node, false);
}

inline void showSubgraph(const INodePtr& node)
{
    showNode(node);

    node->foreachNode([&](const INodePtr& child)
    {
        showNode(child);
        return true;
    });
}

inline void hideNode(const INodePtr& node)
{
    setNodeHidden(node, true);
}

inline void hideSubgraph(const INodePtr& node)
{
    hideNode(node);

    node->foreachNode([&](const INodePtr& child)
    {
        hideNode(child);
        return true;
    });
}

/**
 * greebo: This walker removes all encountered child nodes without
 * traversing each node's children. This deselects all removed nodes as well.
 *
 * Use this to clear all children from a node:
 *
 * NodeRemover walker();
 * node->traverse(walker);
 */
class NodeRemover :
    public scene::NodeVisitor
{
public:
    bool pre(const INodePtr& node) {
        // Copy the node, the reference might point right to
        // the parent's container
        scene::INodePtr copy(node);

        removeNodeFromParent(copy);

        return false;
    }
};

// Copies all visibility flags from the source to the target
inline void assignVisibilityFlagsFromNode(INode& target, const INode& source)
{
    if (source.checkStateFlag(Node::eHidden) && target.supportsStateFlag(Node::eHidden))
    {
        target.enable(Node::eHidden);
    }

    if (source.checkStateFlag(Node::eFiltered) && target.supportsStateFlag(Node::eFiltered))
    {
        target.enable(Node::eFiltered);
    }

    if (source.checkStateFlag(Node::eExcluded) && target.supportsStateFlag(Node::eExcluded))
    {
        target.enable(Node::eExcluded);
    }

    if (source.checkStateFlag(Node::eLayered) && target.supportsStateFlag(Node::eLayered))
    {
        target.enable(Node::eLayered);
    }
}

inline std::pair<Vector3, Vector3> getOriginAndAnglesToLookAtBounds(const AABB& aabb)
{
    Vector3 origin(aabb.origin);

    // Move the camera a bit off the AABB origin
    origin += Vector3(aabb.extents.getLength() * 3, 0, aabb.extents.getLength() * 3);

    // Rotate the camera a bit towards the "ground"
    Vector3 angles(0, 0, 0);
    angles[camera::CAMERA_PITCH] = -40;
    angles[camera::CAMERA_YAW] = 180;

    return std::make_pair(origin, angles);
}

inline std::pair<Vector3, Vector3> getOriginAndAnglesToLookAtNode(const scene::INode& node)
{
    return getOriginAndAnglesToLookAtBounds(node.worldAABB());
}

} // namespace scene
