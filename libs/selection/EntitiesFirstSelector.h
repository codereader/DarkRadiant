#pragma once

#include <map>

#include "iselectable.h"
#include "iselectiontest.h"
#include "ientity.h"

namespace selection
{

/**
 * A Selector implementation sorting entities before primitives.
 * It's used by the selection tests when in SelectionMode::Primitive mode.
 */
class EntitiesFirstSelector :
    public Selector
{
private:
    using SelectableSortedSet = std::multimap<SelectionIntersection, ISelectable*>;

    SelectableSortedSet _entityPool;
    SelectableSortedSet _primitivePool;

    SelectionIntersection _curIntersection;
    ISelectable* _curSelectable;

    // A set of all current ISelectable* candidates, to prevent double-insertions
    // The iterator value points to an element in the SelectableSortedSet
    // to allow for fast lookup and removal.
    std::map<ISelectable*, SelectableSortedSet::iterator> _currentSelectables;

public:
    EntitiesFirstSelector() :
        _curSelectable(nullptr)
    {}

    void pushSelectable(ISelectable& selectable) override
    {
        _curIntersection = SelectionIntersection();
        _curSelectable = &selectable;
    }

    void popSelectable() override
    {
        addSelectable(_curIntersection, _curSelectable);
        _curIntersection = SelectionIntersection();
    }

    void addIntersection(const SelectionIntersection& intersection) override
    {
        _curIntersection.assignIfCloser(intersection);
    }

    void addSelectable(const SelectionIntersection& intersection, ISelectable* selectable)
    {
        if (!intersection.isValid()) return; // skip invalid intersections

        auto existing = _currentSelectables.find(selectable);
        auto isEntity = dynamic_cast<IEntityNode*>(selectable) != nullptr;
        auto& pool = isEntity ? _entityPool : _primitivePool;

        if (existing != _currentSelectables.end())
        {
            // greebo: We had that selectable before, check if the intersection is a better one
            // and update it if necessary. It's possible that the selectable is the parent of
            // two different child primitives, but both may want to add themselves to this pool.
            // To prevent the "worse" primitive from shadowing the "better" one, perform this check.

            // Check if the intersection is better
            if (intersection < existing->second->first)
            {
                // Yes, update the map, remove old stuff first
                pool.erase(existing->second);
                _currentSelectables.erase(existing);
            }
            else
            {
                // The existing intersection is better, we're done here
                return;
            }
        }

        // At this point, the selectable is ready for insertion into the pool
        // Either it's a completely new Selectable, or it is replacing an existing one
        auto result = pool.emplace(intersection, selectable);

        // Memorise the Selectable for fast lookups
        _currentSelectables.emplace(selectable, result);
    }

    bool empty() const override
    {
        return _entityPool.empty() && _primitivePool.empty();
    }

    // Visit each selectable, entities first, then primitives
    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override
    { 
        for (const auto& [_, selectable] : _entityPool)
        {
            functor(selectable);
        }

        for (const auto& [_, selectable] : _primitivePool)
        {
            functor(selectable);
        }
    }
};

}
