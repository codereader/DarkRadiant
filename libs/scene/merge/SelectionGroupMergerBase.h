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
        std::map<std::size_t, std::size_t> baseGroupSizes;

        root->getSelectionGroupManager().foreachSelectionGroup([&](selection::ISelectionGroup& group)
        {
            baseGroupSizes.emplace(group.getId(), group.size());
        });

        _log << "Checking size ordering, got " << baseGroupSizes.size() << " base groups" << std::endl;

        root->foreachNode([&](const INodePtr& node)
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

                // Notify the client code
                actionCallback(node);
            }

            return true;
        });
    }
};

}

}
