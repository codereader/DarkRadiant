#pragma once

#include <sstream>
#include <map>
#include "iselectiongroup.h"
#include "imap.h"
#include "NodeUtils.h"
#include "math/Hash.h"
#include "selectionlib.h"

namespace scene
{

namespace merge
{

class SelectionGroupMergerBase
{
protected:
    std::stringstream _log;

public:
    std::string getLogMessages() const
    {
        return _log.str();
    }

protected:
    using GroupMembers = std::map<std::string, scene::INodePtr>;

    GroupMembers getGroupMemberFingerprints(selection::ISelectionGroup& group)
    {
        GroupMembers members;

        group.foreachNode([&](const INodePtr& member)
        {
            members.emplace(NodeUtils::GetGroupMemberFingerprint(member), member);
        });

        return members;
    }

    // A group fingerprint only consists of the member fingerprints, its ID and the member ordering is irrelevant
    std::string getGroupFingerprint(selection::ISelectionGroup& group)
    {
        std::set<std::string> memberFingerprints;

        group.foreachNode([&](const INodePtr& member)
        {
            memberFingerprints.emplace(NodeUtils::GetGroupMemberFingerprint(member));
        });

        math::Hash hash;

        for (const auto& fingerprint : memberFingerprints)
        {
            hash.addString(fingerprint);
        }

        return hash;
    }

    using NodeFingerprints = std::map<std::string, scene::INodePtr>;

    NodeFingerprints collectNodeFingerprints(const IMapRootNodePtr& root)
    {
        NodeFingerprints result;

        // Collect all source and base nodes for easier lookup
        root->foreachNode([&](const INodePtr& node)
        {
            if (!std::dynamic_pointer_cast<IGroupSelectable>(node)) return true;

            result.emplace(NodeUtils::GetGroupMemberFingerprint(node), node);
            return true;
        });

        return result;
    }

    void ensureGroupSizeOrder(const IMapRootNodePtr& root, const std::function<void(const INodePtr&)>& actionCallback)
    {
        std::map<std::size_t, std::size_t> groupSizes;

        auto& selectionGroupManager = root->getSelectionGroupManager();

        selectionGroupManager.foreachSelectionGroup([&](selection::ISelectionGroup& group)
        {
            groupSizes.emplace(group.getId(), group.size());
        });

        _log << "Checking size ordering, got " << groupSizes.size() << " base groups" << std::endl;

        root->foreachNode([&](const INodePtr& node)
        {
            auto selectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

            if (!selectable) return true;

            auto copiedSet = selectable->getGroupIds();

            std::sort(copiedSet.begin(), copiedSet.end(), [&](std::size_t a, std::size_t b)
            {
                return groupSizes[a] < groupSizes[b];
            });

            // Check if any of the group sizes in this sequence are the same
            if (copiedSet.size() > 1)
            {
                bool groupsWereMerged = false;

                for (auto index = 1; index < copiedSet.size(); ++index)
                {
                    auto curGroupId = copiedSet[index];
                    auto prevGroupId = copiedSet[index - 1];
                    if (groupSizes[prevGroupId] != groupSizes[curGroupId]) continue; // not the same size

                    // Merge this group with the one below, build the union of the two nodes
                    std::set<INodePtr> members1;
                    selectionGroupManager.getSelectionGroup(curGroupId)->foreachNode([&](const INodePtr& member) { members1.insert(member); });

                    std::set<INodePtr> members2;
                    auto groupToKeep = selectionGroupManager.getSelectionGroup(prevGroupId);
                    groupToKeep->foreachNode([&](const INodePtr& member) { members2.insert(member); });

                    std::set<INodePtr> membersNotIn2;
                    std::set_difference(members1.begin(), members1.end(), members2.begin(), members2.end(), std::inserter(membersNotIn2, membersNotIn2.begin()));

                    // Remove the redundant group
                    selectionGroupManager.deleteSelectionGroup(curGroupId);

                    // Remove this group Id from the sorted set
                    copiedSet.erase(std::remove(copiedSet.begin(), copiedSet.end(), curGroupId), copiedSet.end());
                    
                    // Add all missing members to group
                    for (const auto& missingMember : membersNotIn2)
                    {
                        groupToKeep->addNode(missingMember);
                    }

                    // Update the group size map
                    groupSizes[groupToKeep->getId()] = groupToKeep->size();
                    groupsWereMerged = true;

                    // Move the index back one spot, it will be pushed +1 in the for-loop statement
                    --index;
                }

                // We have to sort the groups again after the merge process, to keep the order
                if (groupsWereMerged)
                {
                    std::sort(copiedSet.begin(), copiedSet.end(), [&](std::size_t a, std::size_t b)
                    {
                        return groupSizes[a] < groupSizes[b];
                    });
                }
            }

            if (copiedSet != selectable->getGroupIds())
            {
                _log << "Group membership order in node " << node->name() << " has changed." << std::endl;

                // Re-assign the group to the sorted set
                selection::assignNodeToSelectionGroups(node, copiedSet);

                // Notify the client code
                actionCallback(node);
            }

            return true;
        });
    }
};

}

}
