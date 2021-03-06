#pragma once

#include "SelectionGroupMergerBase.h"

namespace scene
{

namespace merge
{

/**
 * Three-way merger class to detect the changes that have been made
 * to selection groups in the source map, replaying them onto the target scene.
 * 
 * It will try to keep all added grouping information intact, while it will 
 * only remove group links if they haven't been altered in the target map.
 */
class ThreeWaySelectionGroupMerger :
    public SelectionGroupMergerBase
{
public:
    struct Change
    {
        enum class Type
        {
            NodeAddedToGroup,
            TargetGroupAdded,
            TargetGroupRemoved,
            NodeGroupsReordered,
        };

        std::size_t groupId;
        INodePtr member;
        Type type;
    };

private:
    IMapRootNodePtr _baseRoot;
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _targetRoot;

    selection::ISelectionGroupManager& _baseManager;
    selection::ISelectionGroupManager& _sourceManager;
    selection::ISelectionGroupManager& _targetManager;

private:
    // Temporary data only needed during analysis/processing
    NodeFingerprints _targetNodes;

    std::map<std::size_t, std::string> _sourceGroupFingerprints;
    std::set<std::string> _targetGroupFingerprints;

    std::set<std::size_t> _addedSourceGroupIds; // groups that have been added to source
    std::set<std::size_t> _removedSourceGroupIds; // base groups that have been removed in source

    std::set<std::size_t> _modifiedSourceGroupIds; // groups that have been modified in source
    std::set<std::size_t> _modifiedTargetGroupIds; // groups that have been modified in target

    std::vector<Change> _changes;

public:
    ThreeWaySelectionGroupMerger(const IMapRootNodePtr& baseRoot, const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot) :
        _baseRoot(baseRoot),
        _sourceRoot(sourceRoot),
        _targetRoot(targetRoot),
        _baseManager(_baseRoot->getSelectionGroupManager()),
        _sourceManager(_sourceRoot->getSelectionGroupManager()),
        _targetManager(_targetRoot->getSelectionGroupManager())
    {}

    const IMapRootNodePtr& getSourceRoot() const
    {
        return _sourceRoot;
    }

    const IMapRootNodePtr& getTargetRoot() const
    {
        return _targetRoot;
    }

    const IMapRootNodePtr& getBaseRoot() const
    {
        return _baseRoot;
    }

    const std::vector<Change>& getChangeLog() const
    {
        return _changes;
    }
    
    void adjustTargetGroups()
    {
        cleanupWorkingData();
        _changes.clear();
        _log.str(std::string());

        // Collect all node fingerprints for easier lookup
        _targetNodes = collectNodeFingerprints(_targetRoot);
        _log << "Got " << _targetNodes.size() << " in the target map" << std::endl;

        _baseManager.foreachSelectionGroup(
            std::bind(&ThreeWaySelectionGroupMerger::processBaseGroup, this, std::placeholders::_1));

        // We need to know which groups have been added to source and target, respectively
        _sourceManager.foreachSelectionGroup(
            std::bind(&ThreeWaySelectionGroupMerger::processSourceGroup, this, std::placeholders::_1));
        _targetManager.foreachSelectionGroup(
            std::bind(&ThreeWaySelectionGroupMerger::processTargetGroup, this, std::placeholders::_1));

        // Create all source groups and assign new group IDs to not conflict with anything in the target
        addMissingGroupsToTarget();

        // Apply all source group removals if the group has not been modified in target
        removeGroupsFromTarget();

        // Try to replicate all membership changes to groups in the source map
        adjustGroupMemberships();

        // Run a final pass over the node membership to ensure the group sizes are ascending for each node
        // Each group on every node is a superset of any group that was set on that node before
        ensureGroupSizeOrder(_targetRoot, [&](const INodePtr& affectedNode)
        {
            _changes.emplace_back(Change
            {
                0,
                affectedNode,
                Change::Type::NodeGroupsReordered
            });
        });

        cleanupWorkingData();
    }

private:
    void cleanupWorkingData()
    {
        _targetNodes.clear();

        _sourceGroupFingerprints.clear();
        _targetGroupFingerprints.clear();

        _addedSourceGroupIds.clear();

        _removedSourceGroupIds.clear();

        _modifiedSourceGroupIds.clear();
        _modifiedTargetGroupIds.clear();
    }

    void adjustGroupMemberships()
    {
        for (auto id : _modifiedSourceGroupIds)
        {
            auto targetGroup = _targetManager.getSelectionGroup(id);

            if (!targetGroup)
            {
                _log << "The target group with ID " << id << " is no longer present, cannot apply changes." << std::endl;
                continue;
            }

            auto sourceGroup = _sourceManager.getSelectionGroup(id);

            sourceGroup->foreachNode([&](const INodePtr& member)
            {
                auto existingTargetNode = _targetNodes.find(NodeUtils::GetGroupMemberFingerprint(member));

                if (existingTargetNode != _targetNodes.end())
                {
                    _log << "Adding target node to newly created group" << std::endl;
                    targetGroup->addNode(existingTargetNode->second);
                }
            });
        }
    }

    void removeGroupsFromTarget()
    {
        for (auto id : _removedSourceGroupIds)
        {
            // This group can be removed if it has not been modified in target
            if (_modifiedTargetGroupIds.count(id) > 0)
            {
                _log << "The removed source group ID " << id << " has been modified in the target map, won't remove." << std::endl;
                continue;
            }

            _log << "Removing group with ID " << id << " from the target map, as it has been removed in the source" << std::endl;
            _targetManager.deleteSelectionGroup(id);

            _changes.emplace_back(Change
            {
                id,
                INodePtr(),
                Change::Type::TargetGroupRemoved
            });
        }
    }
    
    void addMissingGroupsToTarget()
    {
        for (auto id : _addedSourceGroupIds)
        {
            auto sourceFingerprint = _sourceGroupFingerprints[id];

            // Check if there is an equivalent group in the target map
            if (_targetGroupFingerprints.count(sourceFingerprint))
            {
                _log << "Missing source group has an equivalent group in the target map" << std::endl;
                continue;
            }

            auto targetGroup = _targetManager.createSelectionGroup();

            _changes.emplace_back(Change
            {
                targetGroup->getId(),
                INodePtr(),
                Change::Type::TargetGroupAdded
            });

            _log << "Adding missing source group to the target map: ID=" << targetGroup->getId() << std::endl;

            auto sourceGroup = _sourceManager.getSelectionGroup(id);

            sourceGroup->foreachNode([&](const INodePtr& member)
            {
                auto existingTargetNode = _targetNodes.find(NodeUtils::GetGroupMemberFingerprint(member));

                if (existingTargetNode != _targetNodes.end())
                {
                    _log << "Adding target node to newly created group" << std::endl;
                    targetGroup->addNode(existingTargetNode->second);

                    _changes.emplace_back(Change
                    {
                        targetGroup->getId(),
                        existingTargetNode->second,
                        Change::Type::NodeAddedToGroup
                    });
                }
            });
        }
    }

    void processBaseGroup(selection::ISelectionGroup& group)
    {
        _log << "Processing base group with ID: " << group.getId() << ", size: " << group.size() << std::endl;

        // Check if this group exists in source
        auto sourceGroup = _sourceManager.getSelectionGroup(group.getId());

        if (!sourceGroup)
        {
            _log << "Base group is not present in source: " << group.getId() << std::endl;
            _removedSourceGroupIds.insert(group.getId());
        }
    }

    void processSourceGroup(selection::ISelectionGroup& group)
    {
        _log << "Processing source group with ID: " << group.getId() << ", size: " << group.size() << std::endl;

        auto sourceFingerprint = getGroupFingerprint(group);
        _sourceGroupFingerprints.emplace(group.getId(), sourceFingerprint);

        // Check if this group exists in base
        auto baseGroup = _baseManager.getSelectionGroup(group.getId());

        if (!baseGroup)
        {
            _log << "Source group is not present in base: " << group.getId() << std::endl;
            _addedSourceGroupIds.insert(group.getId());
            return;
        }

        // The base group exists, check if it has the same members
        if (sourceFingerprint != getGroupFingerprint(*baseGroup))
        {
            _modifiedSourceGroupIds.insert(group.getId());
        }
    }

    void processTargetGroup(selection::ISelectionGroup& group)
    {
        _log << "Processing target group with ID: " << group.getId() << ", size: " << group.size() << std::endl;

        auto targetFingerprint = getGroupFingerprint(group);
        _targetGroupFingerprints.emplace(targetFingerprint);

        // Check if this group exists in base
        auto baseGroup = _baseManager.getSelectionGroup(group.getId());

        if (!baseGroup)
        {
            _log << "Target group is not present in base: " << group.getId() << std::endl;
            return;
        }

        // The base group exists, check if it has the same members
        if (targetFingerprint != getGroupFingerprint(*baseGroup))
        {
            _modifiedTargetGroupIds.insert(group.getId());
        }
    }
};

}

}
