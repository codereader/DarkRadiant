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

    std::stringstream _log;
    std::vector<Change> _changes;

private: 
    // Working members that is only needed during processing

    std::map<std::string, INodePtr> _sourceNodes;
    std::map<std::string, INodePtr> _baseNodes;

    std::vector<std::string> _baseLayerNamesToRemove;

    // The remove-node-from-layer action that needs to be executed
    std::vector<std::pair<int, INodePtr>> _baseNodesToRemoveFromLayers;
    // The add-node-to-layer action that needs to be executed
    std::vector<std::pair<int, INodePtr>> _baseNodesToAddToLayers;

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
        cleanupWorkingData();
        _changes.clear();
        _log.str(std::string());

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

        cleanupWorkingData();
    }

private:
    void cleanupWorkingData()
    {
        _sourceNodes.clear();
        _baseNodes.clear();
        _baseLayerNamesToRemove.clear();
        _baseNodesToRemoveFromLayers.clear();
        _baseNodesToAddToLayers.clear();
    }

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
        std::size_t keptNodeCount = 0;

        foreachNodeInLayer(_baseRoot, baseLayerId, [&](const INodePtr& node)
        {
            auto fingerprint = NodeUtils::GetLayerMemberFingerprint(node);

            // All nodes that are also present in the source map are removed from this layer
            // we only keep the base nodes that are preserved during merge
            if (_sourceNodes.count(fingerprint) > 0)
            {
                nodesToRemove.push_back(node);
            }
            else
            {
                keptNodeCount++;
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

        // Remove any layers that turn out empty
        if (keptNodeCount == 0)
        {
            _baseLayerNamesToRemove.push_back(baseLayerName);
        }
    }

    static void foreachNodeInLayer(const INodePtr& root, int layerId, const std::function<void(const INodePtr&)>& functor)
    {
        root->foreachNode([&](const INodePtr& node)
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

    void processSourceLayer(int sourceLayerId, const std::string& sourceLayerName)
    {
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
        auto desiredGroupMembers = getLayerMemberFingerprints(_sourceRoot, sourceLayerId);
        auto currentGroupMembers = getLayerMemberFingerprints(_baseRoot, baseLayerId);
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
    }

    static LayerMembers getLayerMemberFingerprints(const INodePtr& root, int layerId)
    {
        LayerMembers members;

        foreachNodeInLayer(root, layerId, [&](const INodePtr& member)
        {
            members.emplace(NodeUtils::GetLayerMemberFingerprint(member), member);
        });

        return members;
    }
};

}

}