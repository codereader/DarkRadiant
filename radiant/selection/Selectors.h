#ifndef SELECTOR_H_
#define SELECTOR_H_

#include <map>
#include <set>
#include "selectable.h"

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
class SelectionPool : public Selector {
  SelectableSortedSet 	_pool;
  SelectionIntersection	_intersection;
  Selectable* 			_selectable;

public:

	/** greebo: This is called before an entity/patch/brush is 
	 * 			tested against selection to notify the SelectionPool
	 * 			which Selectable we're talking about.	
	 */
	void pushSelectable(Selectable& selectable) {
		_intersection = SelectionIntersection();
		_selectable = &selectable;
	}
  
	/** greebo: Adds the memorised Selectable to the list using the best
	 * 			Intersection that could be found since it has been pushed.
	 */
	void popSelectable() {
		addSelectable(_intersection, _selectable);
		_intersection = SelectionIntersection();
	}

  
	/** greebo: This gets called by the tested items like patches and brushes
	* 		  The brushes test each of their faces against selection and
	* 		  call this method for each of them with the respective Intersection.
	* 		  This method makes sure that the best Intersection of these
	* 		  "subitems" gets added to the list.
	*/
	void addIntersection(const SelectionIntersection& intersection) {
		assign_if_closer(_intersection, intersection);
	}

	/** greebo: This makes sure that only valid Intersections get added, otherwise
	 * 			we would add Selectables that haven't passed the test.
	 */
	void addSelectable(const SelectionIntersection& intersection, Selectable* selectable) {
		if (intersection.valid()) {
			_pool.insert(SelectableSortedSet::value_type(intersection, selectable));
		}
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
