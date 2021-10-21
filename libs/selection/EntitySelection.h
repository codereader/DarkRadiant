#pragma once

#include <list>
#include "inode.h"
#include "ieclass.h"
#include "ientity.h"
#include "iselection.h"
#include "iselectable.h"
#include "imapmerge.h"
#include "CollectiveSpawnargs.h"

namespace selection
{

/**
 * Helper object used by the Entity Inspector to track all selected entities
 * in the scene and keep the list of their spawnargs up to date.
 * 
 * On update() it rescans the entity selection in the scene, triggering
 * the contained CollectiveSpawnargs object to emit its signals. The signals
 * are emitted such that the listening EntityInspector will show the 
 * correct set of keys: the ones with shared values will show just that,
 * the keys with differing values will show a placeholder text instead.
 * 
 * This instance will monitor the selected entities to dispatch all 
 * key value changes to the CollectiveSpawnargs.
 * Selection updates will not take effect until update() is called, which 
 * ideally should happen in an idle processing loop after a selection change.
 * 
 * This class keeps weak references to the scene::Nodes to not interfere with
 * node destruction.
 */
class EntitySelection final :
    public IEntitySelection
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
            // Call the onEntityRemoved method instead of relying on the onKeyErase()
            // invocations when detaching the observer. This allows us to keep some
            // assumptions about entity count in the CollectiveSpawnargs::onKeyErase method
            _spawnargCollection.onEntityRemoved(_entity);

            // Clear the reference to disable the observer callbacks
            auto entity = _entity;
            _entity = nullptr;

            if (!_node.expired())
            {
                // Detaching the observer won't do anything here anymore
                entity->detachObserver(this);
            }
        }

        scene::INodePtr getNode() const
        {
            return _node.lock();
        }

        void onKeyInsert(const std::string& key, EntityKeyValue& value) override
        {
            if (!_entity) return;
            _spawnargCollection.onKeyInsert(_entity, key, value);
        }

        void onKeyChange(const std::string& key, const std::string& value) override
        {
            if (!_entity) return;
            _spawnargCollection.onKeyChange(_entity, key, value);
        }

        void onKeyErase(const std::string& key, EntityKeyValue& value) override
        {
            if (!_entity) return;
            _spawnargCollection.onKeyErase(_entity, key, value);
        }
    };

    std::list<SpawnargTracker> _trackedEntities;

    CollectiveSpawnargs& _spawnargs;

public:
    EntitySelection(CollectiveSpawnargs& spawnargs) :
        _spawnargs(spawnargs)
    {}

    ~EntitySelection()
    {
        _trackedEntities.clear();
    }

    bool empty() const override
    {
        return _trackedEntities.empty();
    }

    std::size_t size() const override
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

    // Returns non-empty reference if all selected entities share the same eclass
    IEntityClassPtr getSingleSharedEntityClass()
    {
        try
        {
            IEntityClassPtr result;

            foreachEntity([&](Entity* entity)
            {
                auto eclass = entity->getEntityClass();

                if (!result)
                {
                    result = std::move(eclass);
                    return;
                }

                if (result != eclass)
                {
                    throw std::runtime_error("Non-unique eclass");
                }
            });

            return result;
        }
        catch (const std::runtime_error&)
        {
            return {};
        }
    }

    std::string getSharedKeyValue(const std::string& key) override
    {
        return _spawnargs.getSharedKeyValue(key);
    }

    void foreachEntity(const std::function<void(Entity*)>& functor) override
    {
        for (auto& tracked : _trackedEntities)
        {
            auto node = tracked.getNode();
            
            if (!node) continue;

            auto entity = Node_getEntity(node);
            assert(entity != nullptr);
            
            if (entity)
            {
                functor(entity);
            }
        }
    }

    // Rescan the selection system for entities to observe
    void update()
    {
        std::set<scene::INodePtr> selectedAndTracked;

        auto trackerCountChanged = false;

        // Untrack all nodes that have been deleted or deselected in the meantime
        for (auto tracked = _trackedEntities.begin(); tracked != _trackedEntities.end();)
        {
            auto node = tracked->getNode();

            if (!node || !Node_isSelected(node))
            {
                // This entity is gone, remove the tracker entry
                _trackedEntities.erase(tracked++);
                trackerCountChanged = true;
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

            if (!entity || selectedAndTracked.count(entity) > 0)
            {
                return; // not an entity or already tracked
            }

            // Not yet registered, create a new tracker
            _trackedEntities.emplace_back(_spawnargs, entity);
            selectedAndTracked.emplace(std::move(entity));
            trackerCountChanged = true;
        });

        if (trackerCountChanged)
        {
            // Notify that the spawnarg collection about newly selected entities
            _spawnargs.onEntityCountChanged();
        }
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
