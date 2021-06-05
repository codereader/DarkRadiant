#pragma once

#include "icomparablenode.h"
#include "imap.h"
#include "iselectiongroup.h"
#include <map>
#include <functional>
#include "NodeUtils.h"
#include "selectionlib.h"

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
class SelectionGroupMerger
{
private:
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _baseRoot;

    selection::ISelectionGroupManager& _sourceManager;
    selection::ISelectionGroupManager& _baseManager;

    std::map<std::string, INodePtr> _sourceNodes;
    std::map<std::string, INodePtr> _baseNodes;

    std::vector<std::size_t> _baseGroupIdsToRemove;

public:
    SelectionGroupMerger(const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& baseRoot) :
        _sourceRoot(sourceRoot),
        _baseRoot(baseRoot),
        _sourceManager(_sourceRoot->getSelectionGroupManager()),
        _baseManager(_baseRoot->getSelectionGroupManager())
    {}

    void adjustBaseGroups()
    {
        // Collect all source and base nodes for easier lookup
        _sourceRoot->foreachNode([&](const INodePtr& node)
        {
            if (!std::dynamic_pointer_cast<IGroupSelectable>(node)) return true;

            _sourceNodes.emplace(NodeUtils::GetGroupMemberFingerprint(node), node);
            return true;
        });

        _baseRoot->foreachNode([&](const INodePtr& node)
        {
            if (!std::dynamic_pointer_cast<IGroupSelectable>(node)) return true;

            _baseNodes.emplace(NodeUtils::GetGroupMemberFingerprint(node), node);
            return true;
        });

        // Remove all base groups not present in the source scene, unless we decide to keep it
        _baseManager.foreachSelectionGroup(
            std::bind(&SelectionGroupMerger::processBaseGroup, this, std::placeholders::_1));

        _sourceManager.foreachSelectionGroup(
            std::bind(&SelectionGroupMerger::processSourceGroup, this, std::placeholders::_1));

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
    using GroupMembers = std::map<std::string, scene::INodePtr>;

    void processBaseGroup(selection::ISelectionGroup& group)
    {
        // Check if there's a counter-part in the source scene
        auto sourceGroup = _sourceManager.getSelectionGroup(group.getId());

        // Existing groups are ok, leave them, conflicts will be resolved in processSourceGroup
        if (sourceGroup) return;

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
            group.removeNode(node);
        }

        if (group.size() < 2)
        {
            _baseGroupIdsToRemove.push_back(group.getId());
        }
    }

    void processSourceGroup(selection::ISelectionGroup& group)
    {
        // Make sure the group exists in the base
        auto baseGroup = _baseManager.getSelectionGroup(group.getId());

        if (!baseGroup)
        {
            baseGroup = _baseManager.createSelectionGroup(group.getId());
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
            currentGroupMembers.begin(), currentGroupMembers.end(), 
            std::back_inserter(membersToBeRemoved), compareFingerprint);
        std::set_difference(currentGroupMembers.begin(), currentGroupMembers.end(), 
            currentGroupMembers.begin(), currentGroupMembers.end(), 
            std::back_inserter(membersToBeAdded), compareFingerprint);

        for (const auto& pair : membersToBeRemoved)
        {
            baseGroup->removeNode(pair.second);
        }

        for (const auto& pair : membersToBeAdded)
        {
            baseGroup->addNode(pair.second);
        }
    }

    GroupMembers getGroupMemberFingerprints(selection::ISelectionGroup& group)
    {
        GroupMembers members;

        group.foreachNode([&](const INodePtr& member)
        {
            members.emplace(NodeUtils::GetGroupMemberFingerprint(member), member);
        });

        return members;
    }

    void ensureGroupSizeOrder()
    {
        std::map<std::size_t, std::size_t> baseGroupSizes;

        _baseManager.foreachSelectionGroup([&](selection::ISelectionGroup& group)
        {
            baseGroupSizes.emplace(group.getId(), group.size());
        });

        _baseRoot->foreachNode([&](const INodePtr& node)
        {
            auto selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

            if (!selectable) return true;

            auto copiedSet = selectable->getGroupIds();
            
            std::sort(copiedSet.begin(), copiedSet.end(), [&](std::size_t a, std::size_t b)
            {
                return baseGroupSizes[a] < baseGroupSizes[b];
            });

            // Re-assign the group to the sorted set
            selection::assignNodeToSelectionGroups(node, copiedSet);

            return true;
        });
    }
};

}

}