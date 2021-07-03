#pragma once

#include <vector>
#include "inode.h"
#include "ientity.h"
#include "icommandsystem.h"
#include "iundo.h"
#include "imap.h"
#include "iselection.h"
#include "imapmerge.h"
#include "scenelib.h"

namespace scene
{

namespace merge
{

// Returns the list of selected merge action nodes
inline std::vector<INodePtr> getSelectedMergeNodes()
{
    std::vector<INodePtr> mergeNodes;

    // Remove the selected nodes
    GlobalSelectionSystem().foreachSelected([&](const INodePtr& node)
    {
        if (node->getNodeType() == INode::Type::MergeAction)
        {
            mergeNodes.push_back(node);
        }
    });

    return mergeNodes;
}

inline std::size_t getNumSelectedMergeNodes()
{
    return getSelectedMergeNodes().size();
}

inline std::shared_ptr<IMergeActionNode> getSingleSelectedConflictNode(const std::vector<INodePtr>& selectedMergeNodes)
{
    if (selectedMergeNodes.size() != 1)
    {
        return std::shared_ptr<IMergeActionNode>();
    }

    auto mergeNode = std::dynamic_pointer_cast<IMergeActionNode>(selectedMergeNodes.front());

    if (mergeNode && mergeNode->getActionType() == ActionType::ConflictResolution)
    {
        return mergeNode;
    }

    return std::shared_ptr<IMergeActionNode>();
}

// Returns exactly one selected merge action node that is representing a conflict resolution
// or an empty reference if the current selection is not suitable
inline std::shared_ptr<IMergeActionNode> getSingleSelectedConflictNode()
{
    return getSingleSelectedConflictNode(getSelectedMergeNodes());
}

// Rejects the selected merge actions by removing them from the scene
// This is an undoable transaction. Removing the nodes from the map will deactivate their actions.
inline void rejectSelectedNodesByDeletion()
{
    UndoableCommand undo("deleteSelectedMergeNodes");

    auto mergeNodes = getSelectedMergeNodes();

    for (const auto& mergeNode : mergeNodes)
    {
        removeNodeFromParent(mergeNode);
    }
}

// Jumps to the next conflict node in the scene, starting from the currently selected one.
// If none is currently selected, the first one will be chosen
inline void focusNextConflictNode()
{
    std::vector<std::shared_ptr<IMergeActionNode>> mergeNodes;

    // Remove the selected nodes
    GlobalMapModule().getRoot()->foreachNode([&](const INodePtr& node)
    {
        if (node->getNodeType() == INode::Type::MergeAction)
        {
            auto mergeNode = std::dynamic_pointer_cast<IMergeActionNode>(node);

            if (mergeNode && mergeNode->getActionType() == ActionType::ConflictResolution)
            {
                mergeNodes.push_back(mergeNode);
            }
        }

        return true;
    });

    if (mergeNodes.empty())
    {
        return;
    }

    INodePtr current;

    if (GlobalSelectionSystem().countSelected() == 1 &&
        GlobalSelectionSystem().ultimateSelected()->getNodeType() == INode::Type::MergeAction)
    {
        current = GlobalSelectionSystem().ultimateSelected();
    }

    auto nextNode = mergeNodes.front();
    auto currentNode = std::find(mergeNodes.begin(), mergeNodes.end(), current);

    if (currentNode != mergeNodes.end() && ++currentNode != mergeNodes.end())
    {
        nextNode = *currentNode;
    }

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(nextNode, true);

    auto originAndAngles = getOriginAndAnglesToLookAtNode(*nextNode->getAffectedNode());
    GlobalCommandSystem().executeCommand("FocusViews", cmd::ArgumentList{ originAndAngles.first, originAndAngles.second });
}

// Returns the name of the entity affected by the given merge action
inline std::string getAffectedEntityName(const IMergeAction::Ptr& action)
{
    auto entity = Node_getEntity(action->getAffectedNode());

    if (entity == nullptr && action->getAffectedNode())
    {
        entity = Node_getEntity(action->getAffectedNode()->getParent());
    }

    return entity ? (entity->isWorldspawn() ? "worldspawn" : entity->getKeyValue("name")) : "?";
}

// Returns the entity key affected by the given merge action
inline std::string getAffectedKeyName(const IMergeAction::Ptr& action)
{
    auto conflictAction = std::dynamic_pointer_cast<IConflictResolutionAction>(action);

    auto keyValueAction = std::dynamic_pointer_cast<IEntityKeyValueMergeAction>(
        conflictAction ? conflictAction->getSourceAction() : action);

    return keyValueAction ? keyValueAction->getKey() : std::string();
}

// Returns the entity key value affected by the given merge action
inline std::string getAffectedKeyValue(const IMergeAction::Ptr& action)
{
    auto conflictAction = std::dynamic_pointer_cast<IConflictResolutionAction>(action);

    auto keyValueAction = std::dynamic_pointer_cast<IEntityKeyValueMergeAction>(
        conflictAction ? conflictAction->getSourceAction() : action);

    return keyValueAction ? keyValueAction->getValue() : std::string();
}

// Resolves the single selected merge conflict by keeping both entity versions.
// The entity in the target map won't receive any changes, whereas the source entity will
// be marked for addition to the target map.
inline void resolveConflictByKeepingBothEntities()
{
    auto conflictNode = getSingleSelectedConflictNode();

    if (conflictNode->getAffectedNode()->getNodeType() != INode::Type::Entity) return;

    conflictNode->foreachMergeAction([&](const IMergeAction::Ptr& action)
    {
        // TODO
    });
}

}

}
