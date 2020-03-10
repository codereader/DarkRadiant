#include "Import.h"

#include <map>

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

// Will map source group IDs to new groups created in the target map
class SelectionGroupRemapper :
	private std::map<std::size_t, selection::ISelectionGroupPtr>
{
private:
	selection::ISelectionGroupManager& _targetGroupManager;

public:
	// The given groupManager will be used to create one corresponding group
	// for each distinct group found in the source nodes
	SelectionGroupRemapper(selection::ISelectionGroupManager& targetGroupManager) :
		_targetGroupManager(targetGroupManager)
	{}

	void assignRemappedGroups(const scene::INodePtr& node, const IGroupSelectable::GroupIds& oldGroupIds)
	{
		rMessage() << "Node " << node->name() << " had the groups " << string::join(oldGroupIds, "|");

		// Get the Groups the source node was assigned to, and add the
		// cloned node to the mapped group, one by one, keeping the order intact
		for (std::size_t id : oldGroupIds)
		{
			// Try to insert the ID, ignore if already exists
			// Get a new mapping for the given group ID
			const selection::ISelectionGroupPtr& mappedGroup = getMappedGroup(id);

			// Assign the new group ID to this clone
			mappedGroup->addNode(node);
		}

		rMessage() << " => " << string::join(std::dynamic_pointer_cast<IGroupSelectable>(node)->getGroupIds(), "|") << std::endl;
	}

	void remapSelectionGroups(const scene::INodePtr& node)
	{
		std::shared_ptr<IGroupSelectable> groupSelectable = std::dynamic_pointer_cast<IGroupSelectable>(node);

		if (groupSelectable)
		{
			auto sourceRoot = node->getRootNode();
			assert(sourceRoot);

			// Save the current set of group IDs
			IGroupSelectable::GroupIds oldGroupIds = groupSelectable->getGroupIds();

			// Remove the node from all its groups
			for (auto id : oldGroupIds)
			{
				auto group = sourceRoot->getSelectionGroupManager().getSelectionGroup(id);

				group->removeNode(node);
			}

			// Assign the new set of groups for this node
			assignRemappedGroups(node, oldGroupIds);
		}
	}

private:
	const selection::ISelectionGroupPtr& getMappedGroup(std::size_t id)
	{
		auto found = emplace(id, selection::ISelectionGroupPtr());

		if (!found.second)
		{
			// We already covered this ID, return the mapped group
			return found.first->second;
		}

		// Insertion was successful, so we didn't cover this ID yet
		found.first->second = _targetGroupManager.createSelectionGroup();

		return found.first->second;
	}
};

class PrimitiveMerger :
	public scene::PrimitiveReparentor
{
private:
	SelectionGroupRemapper _groupRemapper;

public:
	PrimitiveMerger(const scene::INodePtr& newParent, SelectionGroupRemapper& remapper) :
		PrimitiveReparentor(newParent),
		_groupRemapper(remapper)
	{}

	void post(const scene::INodePtr& node) override
	{
		// Base class is doing the reparenting
		PrimitiveReparentor::post(node);

		// Re-generate the group IDs of this node
		_groupRemapper.remapSelectionGroups(node);
		
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

	SelectionGroupRemapper _groupRemapper;

public:
	EntityMerger(const scene::INodePtr& root) :
		_path(scene::Path(root)),
		_groupRemapper(root->getRootNode()->getSelectionGroupManager())
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
				PrimitiveMerger visitor(worldSpawn, _groupRemapper);
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
		// Re-generate the group IDs of this entity node
		_groupRemapper.remapSelectionGroups(node);

		_path.pop();
	}
};

void mergeMap(const scene::INodePtr& node)
{
	// Discard all layer information found in the data to be merged
	// We move everything into the active layer
	{
		scene::LayerList layers;
		layers.insert(GlobalLayerSystem().getActiveLayer());

		scene::AssignNodeToLayersWalker walker(layers);
		node->traverse(walker);
	}

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
