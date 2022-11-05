#pragma once

#include <map>
#include "iselectiontest.h"
#include "iselectable.h"

namespace selection
{

/** 
 * Selector implementation which sorts the incoming selectables by 
 * their respective intersection values.
 *
 * Only selectables with valid intersections are considered.
 *
 * When a selectable is trying to add itself twice to the internal list
 * it is only accepted if its intersection value is better than 
 * the existing one in the pool.
 *
 * The addIntersection() method is called by the tested object in between
 * pushSelectable() and popSelectable(), picking the best Intersection out of the crop.
 */
class SelectionPool :
	public Selector
{
public:
	typedef std::multimap<SelectionIntersection, ISelectable*> SelectableSortedSet;
	typedef SelectableSortedSet::const_iterator const_iterator;

private:
	SelectableSortedSet _pool;
	SelectionIntersection _curIntersection;
	ISelectable* _curSelectable;

	// A set of all current ISelectable* candidates, to prevent double-insertions
	// The iterator value points to an element in the SelectableSortedSet
	// to allow for fast lookup and removal.
	typedef std::map<ISelectable*, SelectableSortedSet::iterator> SelectablesMap;
	SelectablesMap _currentSelectables;

public:
	SelectionPool() :
		_curSelectable(nullptr)
	{}

	// This is called before an entity/patch/brush is tested 
	// to notify the SelectionPool which Selectable we're talking about.
	void pushSelectable(ISelectable& selectable) override
	{
		_curIntersection = SelectionIntersection();
		_curSelectable = &selectable;
	}

	// Adds the memorised Selectable to the list using the best
	// Intersection that could be found since it has been pushed.
	void popSelectable() override
	{
		addSelectable(_curIntersection, _curSelectable);
		_curIntersection = SelectionIntersection();
	}

	/** 
	* This gets called by the tested items like patches and brushes
	* The brushes test each of their faces against selection and
	* call this method for each of them with the respective Intersection.
	* It is ensured that the best Intersection of these
	* "subitems" gets added to the list.
	*/
	void addIntersection(const SelectionIntersection& intersection) override
	{
		_curIntersection.assignIfCloser(intersection);
	}

	/** greebo: This makes sure that only valid Intersections get added, otherwise
	 * 			we would add Selectables that haven't passed the test.
	 */
	void addSelectable(const SelectionIntersection& intersection, ISelectable* selectable)
	{
		if (!intersection.isValid()) return; // skip invalid intersections

		SelectablesMap::iterator existing = _currentSelectables.find(selectable);

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
				_pool.erase(existing->second);
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
		SelectableSortedSet::iterator result = _pool.insert(
			std::make_pair(intersection, selectable)
		);

		// Memorise the Selectable for fast lookups
		_currentSelectables.insert(std::make_pair(selectable, result));
	}

	const_iterator begin() const
	{
		return _pool.begin();
	}

	const_iterator end() const 
	{
		return _pool.end();
	}

	bool empty() const override
	{
		return _pool.empty();
	}

    void foreachSelectable(const std::function<void(ISelectable*)>& functor) override
	{
        for (const auto& [_, selectable] : _pool)
        {
            functor(selectable);
        }
	}
};

}
