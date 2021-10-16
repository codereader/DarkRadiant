#pragma once

#include <list>
#include "inode.h"
#include "ientity.h"
#include "iselection.h"
#include "iselectable.h"
#include "imapmerge.h"
#include "CollectiveSpawnargs.h"

namespace selection
{

// Helper object used by the Entity Inspector to track the 
// selected entities in the scene.
class EntitySelection final
{
private:
    class SpawnargTracker final :
        public Entity::Observer
    {
    private:
        CollectiveSpawnargs& _spawnargCollection;
        Entity* _entity;
        scene::INodeWeakPtr _node;

    public:
        SpawnargTracker(CollectiveSpawnargs& spawnargCollection, const scene::INodePtr& node) :
            _spawnargCollection(spawnargCollection),
            _entity(Node_getEntity(node)),
            _node(node)
        {
            assert(_entity != nullptr);
            _entity->attachObserver(this);
        }

        SpawnargTracker(const SpawnargTracker& other) = delete;
        SpawnargTracker& operator=(const SpawnargTracker& other) = delete;

        ~SpawnargTracker()
        {
            if (_node.expired())
            {
                return; // cannot unsubscribe, the node is gone
            }

            _entity->detachObserver(this);
            _spawnargCollection.cleanupEntity(_entity);
        }

        scene::INodePtr getNode() const
        {
            return _node.lock();
        }

        void onKeyInsert(const std::string& key, EntityKeyValue& value) override
        {
            _spawnargCollection.onKeyInsert(_entity, key, value);
        }

        void onKeyChange(const std::string& key, const std::string& value) override
        {
            _spawnargCollection.onKeyChange(_entity, key, value);
        }

        void onKeyErase(const std::string& key, EntityKeyValue& value) override
        {
            _spawnargCollection.onKeyErase(_entity, key, value);
        }
    };

    std::list<SpawnargTracker> _trackedEntities;

    CollectiveSpawnargs _spawnargs;

public:
    ~EntitySelection()
    {
        _trackedEntities.clear();
    }

    CollectiveSpawnargs& getSpawnargs()
    {
        return _spawnargs;
    }

    std::size_t size() const
    {
        return _trackedEntities.size();
    }

    scene::INodePtr getSingleSelectedEntity() const
    {
        if (size() != 1)
        {
            throw std::runtime_error("Entity selection has count != 1");
        }

        return _trackedEntities.front().getNode();
    }

    // Rescan the selection system for entities to observe
    void update()
    {
        std::set<scene::INodePtr> selectedAndTracked;

        // Untrack all nodes that have been deleted or deselected in the meantime
        for (auto tracked = _trackedEntities.begin(); tracked != _trackedEntities.end();)
        {
            auto node = tracked->getNode();

            if (!node || !Node_isSelected(node))
            {
                // This entity is gone, remove the tracker entry
                _trackedEntities.erase(tracked++);
                continue;
            }

            selectedAndTracked.insert(node);
            ++tracked;
        }

        // Check the selection system for new candidates
        GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& selected)
        {
            // Get the entity this node is attached to - selected a worldspawn brush
            // will let the entity inspector interact with the world entity
            auto entity = getEntityForNode(selected);

            if (!entity || selectedAndTracked.count(selected) > 0)
            {
                return; // not an entity or already tracked
            }

            // Not yet registered, create a new tracker
            _trackedEntities.emplace_back(_spawnargs, entity);
            selectedAndTracked.emplace(std::move(entity));
        });
    }

    void foreachKey(const std::function<void(const std::string&, const CollectiveSpawnargs::KeyValueSet&)>& functor)
    {
        _spawnargs.foreachKey(functor);
    }

private:
    scene::INodePtr getEntityForNode(const scene::INodePtr& selected)
    {
        // The root node must not be selected (this can happen if Invert Selection is
        // activated with an empty scene, or by direct selection in the entity list).
        if (selected->isRoot()) return {};

        if (Node_isEntity(selected))
        {
            return selected; // the node itself is an entity
        }

        // The node is not an entity, check its parent
        auto selectedNodeParent = selected->getParent();

        if (Node_isEntity(selectedNodeParent))
        {
            return selectedNodeParent;
        }

        // Finally, we may encounter merge node types
        if (selected->getNodeType() == scene::INode::Type::MergeAction &&
            GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
        {
            auto mergeAction = std::dynamic_pointer_cast<scene::IMergeActionNode>(selected);
            assert(mergeAction);

            if (mergeAction && Node_isEntity(mergeAction->getAffectedNode()))
            {
                // Use the entity of the merge node
                return mergeAction->getAffectedNode();
            }
        }

        return {}; // nothing of interest
    }
};

}
