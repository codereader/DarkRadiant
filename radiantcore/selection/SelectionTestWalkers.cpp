#include "SelectionTestWalkers.h"

#include "itextstream.h"
#include "iselectable.h"
#include "imodel.h"
#include "igroupnode.h"
#include "iselectiontest.h"
#include "entitylib.h"
#include "debugging/ScenegraphUtils.h"

namespace selection
{

void SelectionTestWalker::printNodeName(const scene::INodePtr& node)
{
	rMessage() << "Node: " << getNameForNodeType(node->getNodeType()) << " ";

	if (node->getNodeType() == scene::INode::Type::Entity)
	{
		rMessage() << " - " << Node_getEntity(node)->getKeyValue("name");
	}

	rMessage() << std::endl;
}

scene::INodePtr SelectionTestWalker::getEntityNode(const scene::INodePtr& node)
{
	return (Node_isEntity(node)) ? node : scene::INodePtr();
}

scene::INodePtr SelectionTestWalker::getParentGroupEntity(const scene::INodePtr& node)
{
	scene::INodePtr parent = node->getParent();

	return (Node_getGroupNode(parent) != NULL) ? parent : scene::INodePtr();
}

bool SelectionTestWalker::entityIsWorldspawn(const scene::INodePtr& node)
{
	return Node_isWorldspawn(node);
}

void SelectionTestWalker::performSelectionTest(const scene::INodePtr& selectableNode,
	const scene::INodePtr& nodeToBeTested)
{
    if (!nodeIsEligibleForTesting(nodeToBeTested))
    {
        return;
    }

    auto selectable = scene::node_cast<ISelectable>(selectableNode);

	if (!selectable) return; // skip non-selectables

	_selector.pushSelectable(*selectable);

	// Test the node for selection, this will add an intersection to the selector
	auto selectionTestable = Node_getSelectionTestable(nodeToBeTested);

	if (selectionTestable)
	{
		selectionTestable->testSelect(_selector, _test);
	}

	_selector.popSelectable();
}

void EntitySelector::testNode(const scene::INodePtr& node)
{
	// Check directly for an entity
	scene::INodePtr entity = getEntityNode(node);

	if (entity == NULL)
	{
		// Skip any models, the parent entity is taking care of the selection test
		if (Node_isModel(node))
		{
			return;
		}

		// Second chance check: is the parent a group node?
		entity = getParentGroupEntity(node);
	}

	// Skip worldspawn in any case
	if (entity == NULL || entityIsWorldspawn(entity)) return;

	// Comment out to hide debugging output
	//printNodeName(node);

	// The entity is the selectable, but the actual node will be tested for selection
	performSelectionTest(entity, node);
}

void PrimitiveSelector::testNode(const scene::INodePtr& node)
{
	// Skip all entities
	if (Node_isEntity(node)) return;

	// Node is not an entity, check parent
	scene::INodePtr parent = getParentGroupEntity(node);

	// Don't select primitives of non-worldspawn entities,
	// the EntitySelector is taking care of that case
	if (parent == NULL || entityIsWorldspawn(parent))
	{
		performSelectionTest(node, node);
	}
}

void GroupChildPrimitiveSelector::testNode(const scene::INodePtr& node)
{
	// Skip all entities
	if (Node_isEntity(node)) return;

	// Node is not an entity, check parent
	scene::INodePtr parent = getParentGroupEntity(node);

	if (parent != NULL && !entityIsWorldspawn(parent))
	{
		performSelectionTest(node, node);
	}
}

void AnySelector::testNode(const scene::INodePtr& node)
{
	scene::INodePtr entity = getEntityNode(node);

	scene::INodePtr candidate;

	if (entity != NULL)
	{
		// skip worldspawn
		if (entityIsWorldspawn(entity)) return;

		// Use this entity as selectable
		candidate = entity;
	}
	else if (Node_isPrimitive(node))
	{
		// Primitives are ok, check for func_static children
		scene::INodePtr parentEntity = getParentGroupEntity(node);

		if (parentEntity != NULL)
		{
			// If this node is a child of worldspawn, it can be directly selected
			// Otherwise this node is a child primitve of a non-worldspawn entity,
			// in which case we want to select the parent entity
			candidate = (entityIsWorldspawn(parentEntity)) ? node : parentEntity;
		}
		else
		{
			// A primitive without parent group entity? Error?
			return; // skip
		}
	}

	// The entity is the selectable, but the actual node will be tested for selection
	performSelectionTest(candidate, node);
}

void ComponentSelector::testNode(const scene::INodePtr& node)
{
	performComponentselectionTest(node);
}

// SelectionSystem::Visitor
void ComponentSelector::visit(const scene::INodePtr& node) const
{
	performComponentselectionTest(node);
}

void ComponentSelector::performComponentselectionTest(const scene::INodePtr& node) const
{
	ComponentSelectionTestablePtr testable = Node_getComponentSelectionTestable(node);

	if (testable)
	{
		testable->testSelectComponents(_selector, _test, _mode);
	}
}

MergeActionSelector::MergeActionSelector(Selector& selector, SelectionTest& test) :
    SelectionTestWalker(selector, test)
{}

void MergeActionSelector::testNode(const scene::INodePtr& node)
{
    performSelectionTest(node, node);
}

bool MergeActionSelector::nodeIsEligibleForTesting(const scene::INodePtr& node)
{
    return node->getNodeType() == scene::INode::Type::MergeAction;
}

}
