#ifndef SELECTOR_H_
#define SELECTOR_H_

#include <map>
#include <set>
#include "iselectiontest.h"
#include "iselectable.h"

typedef std::multimap<SelectionIntersection, Selectable*> SelectableSortedSet;

// A simple set that gets filled after the SelectableSortedSet is populated.
// greebo: I used this to merge two SelectionPools (entities and primitives)
// 		   with a preferred sorting (see RadiantSelectionSystem::Scene_TestSelect())
typedef std::list<Selectable*> SelectablesList;

/* greebo: The SelectionPool contains all the instances that come into question for a selection operation.
 * It can be seen as some kind of stack that can be traversed through
 *
 * The addIntersection() method gets called by the tested object between
 * pushSelectable() and popSelectable() and picks the best Intersection out of the crop.
 *
 */
class SelectionPool :
	public Selector
{
	SelectableSortedSet 	_pool;
	SelectionIntersection	_intersection;
	Selectable* 			_selectable;

	// A set of all current Selectable* candidates, to prevent double-insertions
	// The iterator value points to an element in the SelectableSortedSet
	// to allow for fast removals.
	typedef std::map<Selectable*, SelectableSortedSet::iterator> SelectablesMap;
	SelectablesMap _currentSelectables;

public:

	/** greebo: This is called before an entity/patch/brush is
	 * 			tested against selection to notify the SelectionPool
	 * 			which Selectable we're talking about.
	 */
	void pushSelectable(Selectable& selectable)
	{
		_intersection = SelectionIntersection();
		_selectable = &selectable;
	}

	/** greebo: Adds the memorised Selectable to the list using the best
	 * 			Intersection that could be found since it has been pushed.
	 */
	void popSelectable()
	{
		addSelectable(_intersection, _selectable);
		_intersection = SelectionIntersection();
	}


	/** greebo: This gets called by the tested items like patches and brushes
	* 		  The brushes test each of their faces against selection and
	* 		  call this method for each of them with the respective Intersection.
	* 		  This method makes sure that the best Intersection of these
	* 		  "subitems" gets added to the list.
	*/
	void addIntersection(const SelectionIntersection& intersection)
	{
		assign_if_closer(_intersection, intersection);
	}

	/** greebo: This makes sure that only valid Intersections get added, otherwise
	 * 			we would add Selectables that haven't passed the test.
	 */
	void addSelectable(const SelectionIntersection& intersection, Selectable* selectable)
	{
		if (!intersection.valid()) return; // skip invalid intersections

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
			SelectableSortedSet::value_type(intersection, selectable)
		);

		// Memorise the Selectable for fast lookups
		_currentSelectables.insert(SelectablesMap::value_type(selectable, result));
	}

	typedef SelectableSortedSet::iterator iterator;

	iterator begin() {
		return _pool.begin();
	}

	iterator end() {
		return _pool.end();
	}

	bool failed() {
		return _pool.empty();
	}
};

// =======================================================================================

class BooleanSelector : public Selector {
  bool _selected;
  SelectionIntersection _intersection;
  Selectable* _selectable;
public:
  BooleanSelector() : _selected(false), _selectable(NULL)
  {
  }

  void pushSelectable(Selectable& selectable)
  {
    _intersection = SelectionIntersection();
    _selectable = &selectable;
  }
  void popSelectable()
  {
    if(_intersection.valid())
    {
      _selected = true;
    }
    _intersection = SelectionIntersection();
  }
  void addIntersection(const SelectionIntersection& intersection)
  {
    if(_selectable != NULL && _selectable->isSelected())
    {
      assign_if_closer(_intersection, intersection);
    }
  }

  bool isSelected() {
    return _selected;
  }
};

// =======================================================================================

class BestSelector : public Selector {
  SelectionIntersection _intersection;
  Selectable* _selectable;
  SelectionIntersection _bestIntersection;
  std::list<Selectable*> _bestSelectable;

public:
  BestSelector() : _bestIntersection(SelectionIntersection()), _bestSelectable(0)
  {
  }

  void pushSelectable(Selectable& selectable)
  {
    _intersection = SelectionIntersection();
    _selectable = &selectable;
  }

  void popSelectable()
  {
    if(_intersection.equalEpsilon(_bestIntersection, 0.25f, 0.001f))
    {
      _bestSelectable.push_back(_selectable);
      _bestIntersection = _intersection;
    }
    else if(_intersection < _bestIntersection)
    {
      _bestSelectable.clear();
      _bestSelectable.push_back(_selectable);
      _bestIntersection = _intersection;
    }
    _intersection = SelectionIntersection();
  }

  void addIntersection(const SelectionIntersection& intersection)
  {
    assign_if_closer(_intersection, intersection);
  }

  std::list<Selectable*>& best()
  {
    return _bestSelectable;
  }
};

#endif /*SELECTOR_H_*/
