#pragma once

#include "icomparablenode.h"
#include "imap.h"
#include "ilayer.h"
#include <map>
#include <functional>
#include "NodeUtils.h"

namespace scene
{

namespace merge
{

/**
 * Merger class taking care of adjusting the given target scene such that the
 * the layers match what is defined in the source scene.
 * 
 * It is supposed to run  as a post-process step, after the regular geometry merge 
 * has been finished.
 *
 * It will remove any base layers that are no longer present in the source map
 * unless it's needed by some nodes that are only present in the base map,
 * i.e. they have been explicitly chosen by the user to be kept.
 */
class LayerMerger
{
public:
    struct Change
    {
        enum class Type
        {
            NodeAddedToLayer,
            NodeRemovedFromLayer,
            BaseLayerCreated,
            BaseLayerRemoved,
        };

        int layerId;
        INodePtr member;
        Type type;
    };

private:
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _baseRoot;

    scene::ILayerManager& _sourceManager;
    scene::ILayerManager& _baseManager;

    std::map<std::string, INodePtr> _sourceNodes;
    std::map<std::string, INodePtr> _baseNodes;

    std::vector<std::size_t> _baseLayerIdsToRemove;

    std::stringstream _log;
    std::vector<Change> _changes;

public:

    LayerMerger(const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& baseRoot) :
        _sourceRoot(sourceRoot),
        _baseRoot(baseRoot),
        _sourceManager(_sourceRoot->getLayerManager()),
        _baseManager(_baseRoot->getLayerManager())
    {}

    const IMapRootNodePtr& getSourceRoot() const
    {
        return _sourceRoot;
    }

    const IMapRootNodePtr& getBaseRoot() const
    {
        return _baseRoot;
    }

    std::string getLogMessages() const
    {
        return _log.str();
    }

    const std::vector<Change>& getChangeLog() const
    {
        return _changes;
    }

    void adjustBaseLayers()
    {
        // Collect all source and base nodes for easier lookup
        _sourceRoot->foreachNode([&](const INodePtr& node)
        {
            _sourceNodes.emplace(NodeUtils::GetLayerMemberFingerprint(node), node);
            return true;
        });

        _log << "Got " << _sourceNodes.size() << " nodes in the source map" << std::endl;

        _baseRoot->foreachNode([&](const INodePtr& node)
        {
            _baseNodes.emplace(NodeUtils::GetLayerMemberFingerprint(node), node);
            return true;
        });

        _log << "Got " << _baseNodes.size() << " nodes in the base map" << std::endl;

        _log << "Start Processing base layers" << std::endl;

        // Remove all base layers not present in the source scene, unless we decide to keep it
        _baseManager.foreachLayer(
            std::bind(&LayerMerger::processBaseLayer, this, std::placeholders::_1, std::placeholders::_2));

        _log << "Start Processing source layers" << std::endl;

#if 0
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
#endif
    }

private:
    using LayerMembers = std::map<std::string, scene::INodePtr>;

    void processBaseLayer(int baseLayerId, const std::string& baseLayerName)
    {
        // Check if there's a counter-part in the source scene (by name)
        auto sourceLayer = _sourceManager.getLayerID(baseLayerName);

        // Existing layers are ok, leave them, conflicts will be resolved in processSourceLayer
        if (sourceLayer != -1)
        {
            _log << "Base layer " << baseLayerName << " is present in source too, skipping." << std::endl;
            return;
        }

        // This base layer is no longer present in the source scene,
        // we'll remove it, unless it's referenced by nodes which are base-only
        // which indicates that the base nodes have explicitly been chosen by the user
        // to be kept during the merge operation
        std::vector<INodePtr> nodesToRemove;

        foreachNodeInLayer(baseLayerId, [&](const INodePtr& node)
        {
            auto fingerprint = NodeUtils::GetLayerMemberFingerprint(node);

            // All nodes that are also present in the source map are removed from this layer
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
                baseLayerId,
                node,
                Change::Type::NodeRemovedFromLayer
            });

            _log << "Removing node " << node->name() << " from layer " <<
                baseLayerName << ", since it is not exclusive to the base map." << std::endl;

            node->removeFromLayer(baseLayerId);
        }
    }

    void foreachNodeInLayer(int layerId, const std::function<void(const INodePtr&)>& functor)
    {
        _baseRoot->foreachNode([&](const INodePtr& node)
        {
            if (node->getNodeType() != INode::Type::Entity &&
                node->getNodeType() != INode::Type::Brush &&
                node->getNodeType() != INode::Type::Patch)
            {
                return true;
            }

            if (node->getLayers().count(layerId) > 0)
            {
                functor(node);
            }

            return true;
        });
    }

#if 0
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

    GroupMembers getGroupMemberFingerprints(selection::ISelectionGroup& group)
    {
        GroupMembers members;

        group.foreachNode([&](const INodePtr& member)
        {
            members.emplace(NodeUtils::GetGroupMemberFingerprint(member), member);
        });

        return members;
    }
#endif
};

}

}