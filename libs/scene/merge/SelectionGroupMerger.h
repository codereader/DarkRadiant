#pragma once

#include "icomparablenode.h"
#include "imap.h"
#include "iselectiongroup.h"
#include <map>
#include <functional>
#include "NodeUtils.h"
#include "selectionlib.h"
#include "SelectionGroupMergerBase.h"

namespace scene
{

namespace merge
{

/**
 * Merger class taking care of adjusting the given target scene such that the
 * the groups match what is defined in the source scene.
 * 
 * It tries to keep group links between nodes intact if the nodes are only present 
 * in the base scene (were not removed during the geometry merge phase).
 */
class SelectionGroupMerger :
    public SelectionGroupMergerBase
{
public:
    struct Change
    {
        enum class Type
        {
            NodeAddedToGroup,
            NodeRemovedFromGroup,
            BaseGroupCreated,
            BaseGroupRemoved, 
            NodeGroupsReordered,
        };

        std::size_t groupId;
        INodePtr member;
        Type type;
    };

private:
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _baseRoot;

    selection::ISelectionGroupManager& _sourceManager;
    selection::ISelectionGroupManager& _baseManager;

    std::map<std::string, INodePtr> _sourceNodes;
    std::map<std::string, INodePtr> _baseNodes;

    std::vector<std::size_t> _baseGroupIdsToRemove;

    std::vector<Change> _changes;

public:

    SelectionGroupMerger(const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& baseRoot) :
        _sourceRoot(sourceRoot),
        _baseRoot(baseRoot),
        _sourceManager(_sourceRoot->getSelectionGroupManager()),
        _baseManager(_baseRoot->getSelectionGroupManager())
    {}

    const IMapRootNodePtr& getSourceRoot() const
    {
        return _sourceRoot;
    }

    const IMapRootNodePtr& getBaseRoot() const
    {
        return _baseRoot;
    }

    const std::vector<Change>& getChangeLog() const
    {
        return _changes;
    }

    void adjustBaseGroups()
    {
        // Collect all source and base nodes for easier lookup
        _sourceRoot->foreachNode([&](const INodePtr& node)
        {
            if (!std::dynamic_pointer_cast<IGroupSelectable>(node)) return true;

            _sourceNodes.emplace(NodeUtils::GetGroupMemberFingerprint(node), node);
            return true;
        });

        _log << "Got " << _sourceNodes.size() << " groups in the source map" << std::endl;

        _baseRoot->foreachNode([&](const INodePtr& node)
        {
            if (!std::dynamic_pointer_cast<IGroupSelectable>(node)) return true;

            _baseNodes.emplace(NodeUtils::GetGroupMemberFingerprint(node), node);
            return true;
        });

        _log << "Got " << _baseNodes.size() << " in the base map" << std::endl;

        _log << "Start Processing base groups" << std::endl;

        // Remove all base groups not present in the source scene, unless we decide to keep it
        _baseManager.foreachSelectionGroup(
            std::bind(&SelectionGroupMerger::processBaseGroup, this, std::placeholders::_1));

        _log << "Start Processing source groups" << std::endl;

        _sourceManager.foreachSelectionGroup(
            std::bind(&SelectionGroupMerger::processSourceGroup, this, std::placeholders::_1));

        _log << "Removing " << _baseGroupIdsToRemove.size() << " base groups that have been marked for removal." << std::endl;

        // Remove all base groups that are no longer necessary
        for (auto baseGroupId : _baseGroupIdsToRemove)
        {
            _baseManager.deleteSelectionGroup(baseGroupId);
        }

        // Run a final pass over the node membership to ensure the group sizes are ascending for each node
        // Each group on every node is a superset of any group that was set on that node before
        ensureGroupSizeOrder();
    }

private:
    void processBaseGroup(selection::ISelectionGroup& group)
    {
        // Check if there's a counter-part in the source scene
        auto sourceGroup = _sourceManager.getSelectionGroup(group.getId());

        // Existing groups are ok, leave them, conflicts will be resolved in processSourceGroup
        if (sourceGroup)
        {
            _log << "Base group " << group.getId() << " is present in source too, skipping." << std::endl;
            return;
        }

        // This base group is no longer present in the source scene,
        // we'll remove it, unless it's referenced by nodes which are base-only
        // which indicates that the base nodes have explicitly been chosen by the user
        // to be kept during the merge operation
        std::vector<INodePtr> nodesToRemove;

        group.foreachNode([&](const INodePtr& node)
        {
            auto fingerprint = NodeUtils::GetGroupMemberFingerprint(node);

            // All nodes that are also present in the source map are removed from this group
            // we only keep the base nodes that are preserved during merge
            if (_sourceNodes.count(fingerprint) > 0)
            {
                nodesToRemove.push_back(node);
            }
        });

        for (const auto& node : nodesToRemove)
        {
            _changes.emplace_back(Change
            {
                group.getId(),
                node,
                Change::Type::NodeRemovedFromGroup
            });

            _log << "Removing node " << node->name() << " from group " << 
                group.getId() << ", since it is not exclusive to the base map." << std::endl;

            group.removeNode(node);
        }

        if (group.size() < 2)
        {
            _log << "Base group " << group.getId() << " ends up with less than two members and is marked for removal." << std::endl;

            _changes.emplace_back(Change
            {
                group.getId(),
                INodePtr(),
                Change::Type::BaseGroupRemoved
            });

            _baseGroupIdsToRemove.push_back(group.getId());
        }
    }

    void processSourceGroup(selection::ISelectionGroup& group)
    {
        _log << "Processing source group with ID: " << group.getId() << ", size: " << group.size() << std::endl;

        // Make sure the group exists in the base
        auto baseGroup = _baseManager.getSelectionGroup(group.getId());

        if (!baseGroup)
        {
            _log << "Creating group with ID " << group.getId() << " in the base map" << std::endl;

            baseGroup = _baseManager.createSelectionGroup(group.getId());

            _changes.emplace_back(Change
            {
                group.getId(),
                INodePtr(),
                Change::Type::BaseGroupCreated
            });
        }

        // Ensure the correct members are in the group, if they are available in the map
        auto desiredGroupMembers = getGroupMemberFingerprints(group);
        auto currentGroupMembers = getGroupMemberFingerprints(*baseGroup);
        std::vector<GroupMembers::value_type> membersToBeRemoved;
        std::vector<GroupMembers::value_type> membersToBeAdded;
        
        auto compareFingerprint = [](const GroupMembers::value_type& left, const GroupMembers::value_type& right)
        {
            return left.first < right.first;
        };

        std::set_difference(currentGroupMembers.begin(), currentGroupMembers.end(),
            desiredGroupMembers.begin(), desiredGroupMembers.end(),
            std::back_inserter(membersToBeRemoved), compareFingerprint);
        std::set_difference(desiredGroupMembers.begin(), desiredGroupMembers.end(),
            currentGroupMembers.begin(), currentGroupMembers.end(), 
            std::back_inserter(membersToBeAdded), compareFingerprint);

        _log << "Members to be added: " << membersToBeAdded.size() << ", members to be removed: " << membersToBeRemoved.size() << std::endl;

        for (const auto& pair : membersToBeRemoved)
        {
            // Look up the base node with that fingerprint
            auto baseNode = _baseNodes.find(pair.first);

            if (baseNode == _baseNodes.end())
            {
                _log << "Could not lookup the node " << pair.second->name() << " in the base map for removal" << std::endl;
                continue;
            }

            _log << "Removing node " << baseNode->second->name() << " from group " << baseGroup->getId() << std::endl;
            baseGroup->removeNode(baseNode->second);

            _changes.emplace_back(Change
            {
                group.getId(),
                baseNode->second,
                Change::Type::NodeRemovedFromGroup
            });
        }

        for (const auto& pair : membersToBeAdded)
        {
            // Look up the base node with that fingerprint
            auto baseNode = _baseNodes.find(pair.first);

            if (baseNode == _baseNodes.end())
            {
                _log << "Could not lookup the node " << pair.second->name() << " in the base map for addition" << std::endl;
                continue;
            }

            _log << "Adding node " << baseNode->second->name() << " to group " << baseGroup->getId() << std::endl;
            baseGroup->addNode(baseNode->second);

            _changes.emplace_back(Change
            {
                group.getId(),
                baseNode->second,
                Change::Type::NodeAddedToGroup
            });
        }
    }

    void ensureGroupSizeOrder()
    {
        std::map<std::size_t, std::size_t> baseGroupSizes;

        _baseManager.foreachSelectionGroup([&](selection::ISelectionGroup& group)
        {
            baseGroupSizes.emplace(group.getId(), group.size());
        });

        _log << "Checking size ordering, got " << baseGroupSizes.size() << " base groups" << std::endl;

        _baseRoot->foreachNode([&](const INodePtr& node)
        {
            auto selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

            if (!selectable) return true;

            auto copiedSet = selectable->getGroupIds();
            
            std::sort(copiedSet.begin(), copiedSet.end(), [&](std::size_t a, std::size_t b)
            {
                return baseGroupSizes[a] < baseGroupSizes[b];
            });

            if (copiedSet != selectable->getGroupIds())
            {
                _log << "Group membership order in node " << node->name() << " has changed." << std::endl;

                // Re-assign the group to the sorted set
                selection::assignNodeToSelectionGroups(node, copiedSet);

                _changes.emplace_back(Change
                {
                    0,
                    node,
                    Change::Type::NodeGroupsReordered
                });
            }

            return true;
        });
    }
};

}

}