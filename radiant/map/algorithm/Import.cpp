#include "Import.h"

#include <map>
#include <limits>

#include "imap.h"
#include "imapformat.h"
#include "inamespace.h"
#include "iselectiongroup.h"
#include "ientity.h"
#include "iscenegraph.h"
#include "scene/BasicRootNode.h"
#include "map/algorithm/ChildPrimitives.h"
#include "map/Map.h"
#include "scenelib.h"
#include "entitylib.h"
#include "wxutil/dialog/MessageBox.h"

#include "string/join.h"

namespace map
{

namespace algorithm
{

// Will rewrite the group memberships of visited nodes to not be 
// in conflict with any of the groups present in the target scene
class SelectionGroupRemapper :
	public scene::NodeVisitor
{
private:
	// The group manager of the target scene
	selection::ISelectionGroupManager& _targetGroupManager;

	// Maps old group IDs to new selection groups
	std::map<std::size_t, selection::ISelectionGroupPtr> _groupMap;

	std::size_t _nextNewGroupId;

public:
	SelectionGroupRemapper(selection::ISelectionGroupManager& targetGroupManager) :
		_targetGroupManager(targetGroupManager),
		_nextNewGroupId(0)
	{}

	bool pre(const scene::INodePtr& node) override
	{
		// Check if this node is a group selectable in the first place
		auto groupSelectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

		if (groupSelectable)
		{
			assert(node->getRootNode());

			auto& sourceGroupManager = node->getRootNode()->getSelectionGroupManager();

			// Copy the current set of group IDs locally
			IGroupSelectable::GroupIds oldGroupIds = groupSelectable->getGroupIds();

			// Remove the node from all its groups in the source space before continuing
			for (auto id : oldGroupIds)
			{
				auto group = sourceGroupManager.getSelectionGroup(id);

				group->removeNode(node);
			}

			// Assign the new set of groups for this node
			for (auto id : oldGroupIds)
			{
				// Only do a remap if the group ID is in use in the target space
				auto group = _targetGroupManager.getSelectionGroup(id) ?
					getMappedGroup(id, sourceGroupManager) : sourceGroupManager.getSelectionGroup(id);

				group->addNode(node);
			}

#ifndef NDEBUG
			rMessage() << "Node " << node->name() << " had the groups " << string::join(oldGroupIds, "|");
			rMessage() << " remapped to " << string::join(groupSelectable->getGroupIds(), "|") << std::endl;
#endif
		}

		return true;
	}

private:
	const selection::ISelectionGroupPtr& getMappedGroup(std::size_t idToRemap, selection::ISelectionGroupManager& sourceGroupManager)
	{
		auto found = _groupMap.emplace(idToRemap, selection::ISelectionGroupPtr());

		if (!found.second)
		{
			// We already covered this ID, return the mapped group
			return found.first->second;
		}

		// Insertion was successful, so we didn't cover this ID yet
		// Find a new ID that is not in known to the target manager
		auto newGroupId = generateNonConflictingGroupId();

		// Create this group in the source space
		found.first->second = sourceGroupManager.findOrCreateSelectionGroup(newGroupId);

		return found.first->second;
	}
	
	std::size_t generateNonConflictingGroupId()
	{
		while (++_nextNewGroupId < std::numeric_limits<std::size_t>::max())
		{
			if (!_targetGroupManager.getSelectionGroup(_nextNewGroupId))
			{
				return _nextNewGroupId;
			}
		}

		throw std::runtime_error("Out of group IDs.");
	}
};

class PrimitiveMerger :
	public scene::PrimitiveReparentor
{
public:
	PrimitiveMerger(const scene::INodePtr& newParent) :
		PrimitiveReparentor(newParent)
	{}

	void post(const scene::INodePtr& node) override
	{
		// Base class is doing the reparenting
		PrimitiveReparentor::post(node);

		// After reparenting, highlight the imported node
		Node_setSelected(node, true);
	}
};

class EntityMerger :
	public scene::NodeVisitor
{
private:
	// The target path
	mutable scene::Path _path;

public:
	EntityMerger(const scene::INodePtr& root) :
		_path(scene::Path(root))
	{}

	bool pre(const scene::INodePtr& originalNode) override
	{
		// The removeChildNode below might destroy the instance - push the refcount
		scene::INodePtr node = originalNode;

		// greebo: Check if the visited node is the worldspawn of the other map
		if (Node_isWorldspawn(node))
		{
			// Find the worldspawn of the target map
			const scene::INodePtr& worldSpawn = GlobalMap().getWorldspawn();

			if (!worldSpawn)
			{
				// Set the worldspawn to the new node
				GlobalMap().setWorldspawn(node);

				// greebo: Un-register the node from its previous parent first to be clean
				scene::INodePtr oldParent = node->getParent();

				if (oldParent)
				{
					oldParent->removeChildNode(node);
				}

				// Insert the visited node at the target path
				_path.top()->addChildNode(node);

				_path.push(node);

				// Select all the children of the visited node (these are primitives)
				node->foreachNode([](const scene::INodePtr& child)->bool
				{
					Node_setSelected(child, true);
					return true;
				});
			}
			else
			{
				// The target map already has a worldspawn
				_path.push(worldSpawn);

				// Move all children of this node to the target worldspawn
				PrimitiveMerger visitor(worldSpawn);
				node->traverseChildren(visitor);
			}
		}
		else
		{
			// This is an ordinary entity, not worldspawn

			// greebo: Un-register the entity from its previous root node first to be clean
			scene::INodePtr oldParent = node->getParent();

			if (oldParent)
			{
				oldParent->removeChildNode(node);
			}

			// Insert this node at the target path
			_path.top()->addChildNode(node);
			_path.push(node);

			// Select the visited node
			Node_setSelected(node, true);
		}

		// Only traverse top-level entities, don't traverse the children
		return false;
	}

	void post(const scene::INodePtr& node) override
	{
		_path.pop();
	}
};

void mergeMap(const scene::INodePtr& node)
{
	if (!GlobalSceneGraph().root())
	{
		rError() << "Cannot merge map, no scenegraph root present." << std::endl;
		return;
	}

	// Discard all layer information found in the data to be merged
	// We move everything into the active layer
	{
		scene::LayerList layers;
		layers.insert(GlobalSceneGraph().root()->getLayerManager().getActiveLayer());

		scene::AssignNodeToLayersWalker walker(layers);
		node->traverse(walker);
	}

	// Rewrite the incoming group IDs to not be in conflict with the target scene
	SelectionGroupRemapper remapper(GlobalSceneGraph().root()->getSelectionGroupManager());
	node->traverseChildren(remapper);

	// Now move everything into the target map
	EntityMerger merger(GlobalSceneGraph().root());
	node->traverseChildren(merger);
}

void prepareNamesForImport(const scene::IMapRootNodePtr& targetRoot, const scene::INodePtr& foreignRoot)
{
    const auto& nspace = targetRoot->getNamespace();

    if (nspace)
    {
        // Prepare all names, but do not import them into the namespace. This
        // will happen when nodes are added to the target root later by the caller.
        nspace->ensureNoConflicts(foreignRoot);
    }
}

MapFormatPtr determineMapFormat(std::istream& stream, const std::string& type)
{
	// Get all registered map formats matching the extension
	auto availableFormats = type.empty() ?
		GlobalMapFormatManager().getAllMapFormats() :
		GlobalMapFormatManager().getMapFormatList(type);

	MapFormatPtr format;

	for (const auto& candidate : availableFormats)
	{
		// Rewind the stream before passing it to the format for testing
		// Map format valid, rewind the stream
		stream.seekg(0, std::ios_base::beg);

		if (candidate->canLoad(stream))
		{
			format = candidate;
			break;
		}
	}

	// Rewind the stream when we're done
	stream.seekg(0, std::ios_base::beg);

	return format;
}

MapFormatPtr determineMapFormat(std::istream& stream)
{
	return determineMapFormat(stream, std::string());
}

class SimpleMapImportFilter :
    public IMapImportFilter
{
private:
    scene::IMapRootNodePtr _root;

public:
    SimpleMapImportFilter() :
        _root(new scene::BasicRootNode)
    {}

    const scene::IMapRootNodePtr& getRootNode() const
    {
        return _root;
    }

    bool addEntity(const scene::INodePtr& entityNode)
    {
        _root->addChildNode(entityNode);
        return true;
    }

    bool addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity)
    {
        if (Node_getEntity(entity)->isContainer())
        {
            entity->addChildNode(primitive);
            return true;
        }
        
        return false;
    }
};

void importFromStream(std::istream& stream)
{
	GlobalSelectionSystem().setSelectedAll(false);

    // Instantiate the default import filter
    SimpleMapImportFilter importFilter;

    try
    {
        auto format = determineMapFormat(stream);

        if (!format)
        {
            throw IMapReader::FailureException(_("Unknown map format"));
        }

        auto reader = format->getMapReader(importFilter);

        // Start parsing
        reader->readFromStream(stream);

        // Prepare child primitives
        addOriginToChildPrimitives(importFilter.getRootNode());

        // Adjust all new names to fit into the existing map namespace
        prepareNamesForImport(GlobalMap().getRoot(), importFilter.getRootNode());

        mergeMap(importFilter.getRootNode());
    }
    catch (IMapReader::FailureException& ex)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Failure reading map from clipboard:\n{0}"), ex.what()));

        // Clear out the root node, otherwise we end up with half a map
        scene::NodeRemover remover;
        importFilter.getRootNode()->traverseChildren(remover);
    }
}

}

}
