#pragma once

#include <vector>
#include "inode.h"
#include "ientity.h"
#include "icommandsystem.h"
#include "iundo.h"
#include "imap.h"
#include "iselection.h"
#include "imapmerge.h"
#include "itextstream.h"
#include "scenelib.h"
#include "MergeAction.h"
#include "MergeActionNode.h"

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

inline bool isKeyValueConflictAction(const IConflictResolutionAction::Ptr& conflictAction)
{
    return conflictAction && 
           conflictAction->getConflictType() == ConflictType::ModificationOfRemovedKeyValue ||
           conflictAction->getConflictType() == ConflictType::RemovalOfModifiedKeyValue ||
           conflictAction->getConflictType() == ConflictType::SettingKeyToDifferentValue;
}

inline bool actionIsTargetingKeyValue(const IMergeAction::Ptr& action)
{
    if (action->getType() == ActionType::AddKeyValue ||
        action->getType() == ActionType::RemoveKeyValue ||
        action->getType() == ActionType::ChangeKeyValue)
    {
        return true;
    }

    // Conflict actions can be targeting key values too
    if (action->getType() == ActionType::ConflictResolution &&
        isKeyValueConflictAction(std::dynamic_pointer_cast<IConflictResolutionAction>(action)))
    {
        return true;
    }

    return false;
}

// Resolves the single selected merge conflict by keeping both entity versions.
// The entity in the target map won't receive any changes, whereas the source entity will
// be marked for addition to the target map.
inline void resolveConflictByKeepingBothEntities()
{
    auto operation = GlobalMapModule().getActiveMergeOperation();

    if (!operation)
    {
        rError() << "Cannot resolve a conflict without an active merge operation" << std::endl;
        return;
    }

    auto mergeNodes = getSelectedMergeNodes();

    for (const auto& node : mergeNodes)
    {
        auto mergeNode = std::dynamic_pointer_cast<IMergeActionNode>(node);

        // We are only looking for conflict resolution types
        if (mergeNode->getAffectedNode()->getNodeType() != INode::Type::Entity ||
            mergeNode->getActionType() != ActionType::ConflictResolution)
        {
            return;
        }

        // Get all the actions from the conflict node that are targeting key values
        // and extract the source/target entity pairs from the conflict actions
        
        // We expect that we only get one source/target combination from a single node
        std::set<std::pair<INodePtr, INodePtr>> sourceAndTargetEntities;
        std::vector<IMergeAction::Ptr> keyValueActions;

        mergeNode->foreachMergeAction([&](const IMergeAction::Ptr& action)
        {
            if (actionIsTargetingKeyValue(action))
            {
                keyValueActions.push_back(action);
            }

            auto conflictAction = std::dynamic_pointer_cast<IConflictResolutionAction>(action);

            if (conflictAction)
            {
                sourceAndTargetEntities.insert(
                    std::make_pair(conflictAction->getConflictingSourceEntity(), conflictAction->getConflictingTargetEntity()));
            }
        });

        if (sourceAndTargetEntities.size() != 1)
        {
            rWarning() << "The conflict action node contains a weird combination and source and target entity nodes, cannot resolve" << std::endl;
            return;
        }

        // Add the action to import the source node as a whole
        const auto& pair = *sourceAndTargetEntities.begin();
        auto addSourceEntityAction = std::make_shared<AddEntityAction>(pair.first, pair.second->getRootNode());

        operation->addAction(addSourceEntityAction); // TODO: Observer pattern to notify Map class

        // Add a new merge action node for the addition to the target scene
        auto mergeActionNode = std::make_shared<RegularMergeActionNode>(addSourceEntityAction);
        GlobalMapModule().getRoot()->addChildNode(mergeActionNode);

        // Remove the conflict resolution node
        removeNodeFromParent(mergeNode);
    }
}

}

}
