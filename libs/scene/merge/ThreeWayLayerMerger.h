#pragma once

#include "icomparablenode.h"
#include "imap.h"
#include "ilayer.h"
#include <map>
#include <functional>
#include "NodeUtils.h"
#include "LayerMergerBase.h"

namespace scene
{

namespace merge
{

/**
 * Three-way layer merger class which will apply changes to layers in the source scene
 * to the target scene (compared against a common base).
 * 
 * It is supposed to run  as a post-process step, after the regular geometry merge 
 * has been finished.
 * 
 * Layers are matched up by name, IDs are irrelevant for this algorithm
 * 
 * Strategy for layers deleted in source:
 * - If target didn't modify the layer, accept the deletion
 * - If target modified the layer: accept the deletion only if the target exclusively removed nodes from it
 * 
 * Strategy for layers added to source:
 * - If the name doesn't exist in target yet: add it
 * - If the name is already in use in the target: 
 *   - If the layers perfectly match up, do nothing
 *   - If the layers don't match, add the layer from source with a new, non-conflicting name to the target
 * 
 * Strategy for layers modified in source:
 * - If layer in target has not been modified: accept all changes
 * - If layer in target has been modified: accept all changes
 * - If layer has been deleted in target:
 *   - If the source exclusively removed nodes from it, the deletion will persist
 *   - If the source made any additions to the layer, the layer will be re-created with all its members, as in source
 * 
 * A membership change is acceptable in target if the node is still available there. If the node
 * is no longer present in the target map, all operations affecting it will be ignored.
 */
class ThreeWayLayerMerger :
    public LayerMergerBase
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
    IMapRootNodePtr _baseRoot;
    IMapRootNodePtr _sourceRoot;
    IMapRootNodePtr _targetRoot;

    scene::ILayerManager& _baseManager;
    scene::ILayerManager& _sourceManager;
    scene::ILayerManager& _targetManager;

    std::vector<Change> _changes;

private: 
    // Working members that is only needed during processing

    // A single atomic change made to a layer
    struct LayerChange
    {
        enum class Type
        {
            NodeAddition,
            NodeRemoval,
        };
        
        Type type;
        INodePtr node;
        std::string fingerprint;
    };

    std::map<std::string, INodePtr> _sourceNodes;
    std::map<std::string, INodePtr> _targetNodes;

    std::vector<std::string> _baseLayerNamesRemovedInSource;
    std::vector<std::string> _baseLayerNamesRemovedInTarget;

    std::vector<std::string> _addedTargetLayerNames;

    std::map<std::string, std::vector<LayerChange>> _targetLayerChanges;

    std::map<int, LayerMembers> _baseLayerMembers;

    // The remove-node-from-layer action that needs to be executed
    std::vector<std::pair<int, INodePtr>> _baseNodesToRemoveFromLayers;
    // The add-node-to-layer action that needs to be executed
    std::vector<std::pair<int, INodePtr>> _baseNodesToAddToLayers;

public:

    ThreeWayLayerMerger(const IMapRootNodePtr& baseRoot, const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot) :
        _baseRoot(baseRoot),
        _sourceRoot(sourceRoot),
        _targetRoot(targetRoot),
        _baseManager(_baseRoot->getLayerManager()),
        _sourceManager(_sourceRoot->getLayerManager()),
        _targetManager(_targetRoot->getLayerManager())
    {}

    const IMapRootNodePtr& getTargetRoot() const
    {
        return _targetRoot;
    }

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

    void adjustTargetLayers()
    {
        cleanupWorkingData();
        _changes.clear();
        _log.str(std::string());

        // Collect all source and target nodes for easier lookup
        _sourceRoot->foreachNode([&](const INodePtr& node)
        {
            _sourceNodes.emplace(NodeUtils::GetLayerMemberFingerprint(node), node);
            return true;
        });

        _log << "Got " << _sourceNodes.size() << " nodes in the source map" << std::endl;

        _targetRoot->foreachNode([&](const INodePtr& node)
        {
            _targetNodes.emplace(NodeUtils::GetLayerMemberFingerprint(node), node);
            return true;
        });

        _log << "Got " << _targetNodes.size() << " nodes in the base map" << std::endl;

        _log << "Analysing missing base layers" << std::endl;

        _baseManager.foreachLayer(
            std::bind(&ThreeWayLayerMerger::analyseBaseLayer, this, std::placeholders::_1, std::placeholders::_2));

        _log << "Analysing target layers with respect to base" << std::endl;

        _targetManager.foreachLayer(
            std::bind(&ThreeWayLayerMerger::analyseTargetLayer, this, std::placeholders::_1, std::placeholders::_2));
#if 0
        _log << "Start Processing source layers" << std::endl;

        _sourceManager.foreachLayer(
            std::bind(&LayerMerger::processSourceLayer, this, std::placeholders::_1, std::placeholders::_2));

        // Execute the actions we found during source layer processing
        // Add to new layers first, since removing a node from its last layer might not succeed otherwise
        for (const auto& pair : _baseNodesToAddToLayers)
        {
            _log << "Adding node " << pair.second->name() << " to layer " << pair.first << std::endl;

            pair.second->addToLayer(pair.first);

            _changes.emplace_back(Change
            {
                pair.first,
                pair.second,
                Change::Type::NodeAddedToLayer
            });
        }

        for (const auto& pair : _baseNodesToRemoveFromLayers)
        {
            _log << "Removing node " << pair.second->name() << " from layer " << pair.first << std::endl;

            pair.second->removeFromLayer(pair.first);

            _changes.emplace_back(Change
            {
                pair.first,
                pair.second,
                Change::Type::NodeRemovedFromLayer
            });
        }

        _log << "Removing " << _baseLayerNamesToRemove.size() << " base layers that have been marked for removal." << std::endl;

        // Remove all base layers that are no longer necessary
        for (auto baseLayerName : _baseLayerNamesToRemove)
        {
            auto baseLayerId = _baseManager.getLayerID(baseLayerName);
            assert(baseLayerId != -1);

            _baseManager.deleteLayer(baseLayerName);

            _changes.emplace_back(Change
            {
                baseLayerId,
                INodePtr(),
                Change::Type::BaseLayerRemoved
            });
        }
#endif
        cleanupWorkingData();
    }

private:
    void cleanupWorkingData()
    {
        _sourceNodes.clear();
        _targetNodes.clear();
        _baseLayerNamesRemovedInSource.clear();

        _baseNodesToRemoveFromLayers.clear();
        _baseNodesToAddToLayers.clear();
    }

    void analyseBaseLayer(int baseLayerId, const std::string& baseLayerName)
    {
        // Collect the member fingerprints
        _baseLayerMembers.emplace(baseLayerId, GetLayerMemberFingerprints(_baseRoot, baseLayerId));

        // Check if there's a counter-part in the source scene (by name)
        auto sourceLayer = _sourceManager.getLayerID(baseLayerName);

        // Existing layers are ok, leave them, conflicts will be resolved in source layer processing
        if (sourceLayer != -1)
        {
            _log << "Base layer " << baseLayerName << " is present in source too, skipping." << std::endl;
        }
        else
        {
            // This base layer is no longer present in the source scene
            _baseLayerNamesRemovedInSource.push_back(baseLayerName);
        }

        // Check if there's a counter-part in the target scene (by name)
        auto targetLayer = _targetManager.getLayerID(baseLayerName);

        // Existing layers are ok, leave them, conflicts will be resolved in target layer processing
        if (targetLayer != -1)
        {
            _log << "Base layer " << baseLayerName << " is present in target too, skipping." << std::endl;
        }
        else
        {
            // This base layer is no longer present in the target scene
            _baseLayerNamesRemovedInTarget.push_back(baseLayerName);
        }
    }

    void analyseTargetLayer(int targetLayerId, const std::string& targetLayerName)
    {
        // Check if there's a counter-part in the base scene (by name)
        auto baseLayer = _baseManager.getLayerID(targetLayerName);

        if (baseLayer != -1)
        {
            _log << "Target layer " << targetLayerName << " is present in source too, checking differences." << std::endl;

            // Collect member fingerprints of target and base
            auto targetMembers = GetLayerMemberFingerprints(_targetRoot, targetLayerId);
            assert(_baseLayerMembers.count(targetLayerId) == 1);

            const auto& baseMembers = _baseLayerMembers[targetLayerId];

            _targetLayerChanges.emplace(targetLayerName, getLayerChanges(targetMembers, baseMembers));
        }
        else
        {
            // This layer is not present in the base scene
            _addedTargetLayerNames.push_back(targetLayerName);
        }
    }

    std::vector<LayerChange> getLayerChanges(const LayerMembers& changed, const LayerMembers& base)
    {
        std::vector<LayerChange> result;

        auto compareFingerprint = [](const LayerMembers::value_type& left, const LayerMembers::value_type& right)
        {
            return left.first < right.first;
        };

        std::vector<LayerMembers::value_type> addedMembers;
        std::vector<LayerMembers::value_type> removedMembers;

        std::set_difference(changed.begin(), changed.end(), base.begin(), base.end(),
            std::back_inserter(addedMembers), compareFingerprint);

        std::set_difference(base.begin(), base.end(), changed.begin(), changed.end(),
            std::back_inserter(removedMembers), compareFingerprint);

        _log << "Found " << addedMembers.size() << " new members and " << removedMembers.size() << " removed members" << std::endl;

        for (const auto& pair : addedMembers)
        {
            result.emplace_back(LayerChange
            {
                LayerChange::Type::NodeAddition, pair.second, pair.first
            });
        }

        for (const auto& pair : removedMembers)
        {
            result.emplace_back(LayerChange
            {
                LayerChange::Type::NodeRemoval, pair.second, pair.first
            });
        }

        return result;
    }

    void processSourceLayer(int sourceLayerId, const std::string& sourceLayerName)
    {
#if 0
        _log << "Processing source layer with ID: " << sourceLayerId << " and name: " << sourceLayerName << std::endl;

        // Make sure the layer exists in the base
        auto baseLayerId = _baseManager.getLayerID(sourceLayerName);

        if (baseLayerId == -1)
        {
            _log << "Creating layer with ID " << sourceLayerId << " in the base map" << std::endl;

            // We only care about names, so don't specify any IDs here
            baseLayerId = _baseManager.createLayer(sourceLayerName);

            _changes.emplace_back(Change
            {
                baseLayerId,
                INodePtr(),
                Change::Type::BaseLayerCreated
            });
        }

        // Ensure the correct members are in the group, if they are available in the map
        auto desiredGroupMembers = GetLayerMemberFingerprints(_sourceRoot, sourceLayerId);
        auto currentGroupMembers = GetLayerMemberFingerprints(_baseRoot, baseLayerId);
        std::vector<LayerMembers::value_type> membersToBeRemoved;
        std::vector<LayerMembers::value_type> membersToBeAdded;

        auto compareFingerprint = [](const LayerMembers::value_type& left, const LayerMembers::value_type& right)
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

            // We keep all the layer memberships of nodes that the user wants to keep
            if (_sourceNodes.count(pair.first) == 0)
            {
                continue;
            }

            _log << "Marking node " << baseNode->second->name() << " for removal from layer " << sourceLayerName << std::endl;
            _baseNodesToRemoveFromLayers.emplace_back(std::make_pair(baseLayerId, baseNode->second));
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

            _log << "Marking node " << baseNode->second->name() << " for addition to layer " << sourceLayerName << std::endl;
            _baseNodesToAddToLayers.emplace_back(std::make_pair(baseLayerId, baseNode->second));
        }
#endif
    }
};

}

}
