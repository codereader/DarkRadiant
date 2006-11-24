#ifndef SELECTOR_H_
#define SELECTOR_H_

#include <map>
#include "selectable.h"

typedef std::multimap<SelectionIntersection, Selectable*> SelectableSortedSet;

/* greebo: The SelectionPool contains all the instances that come into question for a selection operation.
 * It can be seen as some kind of stack that can be traversed through  
 */

class SelectionPool : public Selector {
  SelectableSortedSet 	_pool;
  SelectionIntersection	_intersection;
  Selectable* 			_selectable;

public:

  void pushSelectable(Selectable& selectable) {
    _intersection = SelectionIntersection();
    _selectable = &selectable;
  }
  
  void popSelectable() {
    addSelectable(_intersection, _selectable);
    _intersection = SelectionIntersection();
  }
  
  void addIntersection(const SelectionIntersection& intersection) {
    assign_if_closer(_intersection, intersection);
  }
  
  void addSelectable(const SelectionIntersection& intersection, Selectable* selectable) {
    if(intersection.valid()) {
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
  BooleanSelector() : _selected(false)
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
    if(_selectable->isSelected())
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
