#ifndef SELECTOR_H_
#define SELECTOR_H_

#include <map>
#include "selectable.h"

typedef std::multimap<SelectionIntersection, Selectable*> SelectableSortedSet;

class SelectionPool : public Selector
{
  SelectableSortedSet 	m_pool;
  SelectionIntersection	m_intersection;
  Selectable* 			m_selectable;

public:

  void pushSelectable(Selectable& selectable) {
    m_intersection = SelectionIntersection();
    m_selectable = &selectable;
  }
  
  void popSelectable() {
    addSelectable(m_intersection, m_selectable);
    m_intersection = SelectionIntersection();
  }
  
  void addIntersection(const SelectionIntersection& intersection) {
    assign_if_closer(m_intersection, intersection);
  }
  
  void addSelectable(const SelectionIntersection& intersection, Selectable* selectable) {
    if(intersection.valid()) {
      m_pool.insert(SelectableSortedSet::value_type(intersection, selectable));
    }
  }

  typedef SelectableSortedSet::iterator iterator;

  iterator begin() {
    return m_pool.begin();
  }
  
  iterator end() {
    return m_pool.end();
  }

  bool failed() {
    return m_pool.empty();
  }
};

#endif /*SELECTOR_H_*/
