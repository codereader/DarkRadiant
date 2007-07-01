/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined (INCLUDED_TRAVERSELIB_H)
#define INCLUDED_TRAVERSELIB_H

#include "debugging/debugging.h"

#include "scenelib.h"
#include "undolib.h"
#include "container/container.h"

#include <list>
#include <vector>
#include <algorithm>

class TraversableObserverInsertOutputIterator 
{
protected:
  scene::Traversable::Observer* m_observer;
public:
  typedef std::output_iterator_tag iterator_category;
  typedef void difference_type;
  typedef void value_type;
  typedef void pointer;
  typedef void reference;

  TraversableObserverInsertOutputIterator(scene::Traversable::Observer* observer) 
    : m_observer(observer)
  {
  }
  TraversableObserverInsertOutputIterator& operator=(const scene::INodePtr node)
  { 
    m_observer->insertChild(node);
    return *this;
  }
  TraversableObserverInsertOutputIterator& operator*() { return *this; }
  TraversableObserverInsertOutputIterator& operator++() { return *this; }
  TraversableObserverInsertOutputIterator& operator++(int) { return *this; }
};

class TraversableObserverEraseOutputIterator 
{
protected:
  scene::Traversable::Observer* m_observer;
public:
  typedef std::output_iterator_tag iterator_category;
  typedef void difference_type;
  typedef void value_type;
  typedef void pointer;
  typedef void reference;

  TraversableObserverEraseOutputIterator(scene::Traversable::Observer* observer) 
    : m_observer(observer)
  {
  }
  TraversableObserverEraseOutputIterator& operator=(const scene::INodePtr node)
  { 
    m_observer->eraseChild(node);
    return *this;
  }
  TraversableObserverEraseOutputIterator& operator*() { return *this; }
  TraversableObserverEraseOutputIterator& operator++() { return *this; }
  TraversableObserverEraseOutputIterator& operator++(int) { return *this; }
};

typedef UnsortedSet<scene::INodePtr> UnsortedNodeSet;

/// \brief Calls \p observer->\c insert for each node that exists only in \p other and \p observer->\c erase for each node that exists only in \p self
inline void nodeset_diff(const UnsortedNodeSet& self, const UnsortedNodeSet& other, scene::Traversable::Observer* observer)
{
  std::vector<scene::INodePtr> sorted(self.begin(), self.end());
  std::vector<scene::INodePtr> other_sorted(other.begin(), other.end());

  std::sort(sorted.begin(), sorted.end());
  std::sort(other_sorted.begin(), other_sorted.end());

  std::set_difference(sorted.begin(), sorted.end(), other_sorted.begin(), other_sorted.end(), TraversableObserverEraseOutputIterator(observer));
  std::set_difference(other_sorted.begin(), other_sorted.end(), sorted.begin(), sorted.end(), TraversableObserverInsertOutputIterator(observer));
}

/// \brief A sequence of node references which notifies an observer of inserts and deletions, and uses the global undo system to provide undo for modifications.
class TraversableNodeSet : 
	public scene::Traversable
{
  UnsortedNodeSet m_children;
  UndoableObject<TraversableNodeSet> m_undo;
  Observer* m_observer;

  void copy(const TraversableNodeSet& other)
  {
    m_children = other.m_children;
  }
  void notifyInsertAll()
  {
    if(m_observer)
    {
      for(UnsortedNodeSet::iterator i = m_children.begin(); i != m_children.end(); ++i)
      {
        m_observer->insertChild(*i);
      }
    }
  }
  void notifyEraseAll()
  {
    if(m_observer)
    {
      for(UnsortedNodeSet::iterator i = m_children.begin(); i != m_children.end(); ++i)
      {
        m_observer->eraseChild(*i);
      }
    }
  }
public:
  TraversableNodeSet()
    : m_undo(*this), m_observer(0)
  {
  }
  TraversableNodeSet(const TraversableNodeSet& other)
    : scene::Traversable(other), m_undo(*this), m_observer(0)
  {
    copy(other);
    notifyInsertAll();
  }
  ~TraversableNodeSet()
  {
  	//std::cout << "TraversableNodeSet destructed.\n";
    notifyEraseAll();
  }
  TraversableNodeSet& operator=(const TraversableNodeSet& other)
  {
#if 1 // optimised change-tracking using diff algorithm
    if(m_observer)
    {
      nodeset_diff(m_children, other.m_children, m_observer);
    }
    copy(other);
#else
    TraversableNodeSet tmp(other);
    tmp.swap(*this);
#endif
    return *this;
  }
  void swap(TraversableNodeSet& other)
  {
    std::swap(m_children, other.m_children);
    std::swap(m_observer, other.m_observer);
  }

  void attach(Observer* observer)
  {
    ASSERT_MESSAGE(m_observer == 0, "TraversableNodeSet::attach: observer cannot be attached");
    m_observer = observer;
    notifyInsertAll();
  }
  void detach(Observer* observer)
  {
    ASSERT_MESSAGE(m_observer == observer, "TraversableNodeSet::detach: observer cannot be detached");
    notifyEraseAll();
    m_observer = 0;
  }
  /// \brief \copydoc scene::Traversable::insert()
  void insert(scene::INodePtr node)
  {
  	//std::cout << "TraversableNodeSet: Inserting child: " << node.get() << "\n";
    //ASSERT_MESSAGE(&node != 0, "TraversableNodeSet::insert: sanity check failed");
    m_undo.save();

    //ASSERT_MESSAGE(m_children.find(node) == m_children.end(), "TraversableNodeSet::insert - element already exists");

    m_children.insert(node);

    if(m_observer)
    {
      m_observer->insertChild(node);
    }
  }
  /// \brief \copydoc scene::Traversable::erase()
  void erase(scene::INodePtr node)
  {
  	//std::cout << "TraversableNodeSet: Erasing child: " << node.get() << "\n";
    ASSERT_MESSAGE(&node != 0, "TraversableNodeSet::erase: sanity check failed");
    m_undo.save();

    //ASSERT_MESSAGE(m_children.find(node) != m_children.end(), "TraversableNodeSet::erase - failed to find element");

    if(m_observer)
    {
      m_observer->eraseChild(node);
    }

    m_children.erase(node);
  }
  /// \brief \copydoc scene::Traversable::traverse()
  void traverse(const Walker& walker)
  {
    UnsortedNodeSet::iterator i = m_children.begin();
    while(i != m_children.end())
    {
      // post-increment the iterator
      Node_traverseSubgraph(*i++, walker);
      // the Walker can safely remove the current node from
      // this container without invalidating the iterator
    }
  }
  /// \brief \copydoc scene::Traversable::empty()
  bool empty() const
  {
    return m_children.empty();
  }

  void instanceAttach(MapFile* map)
  {
    m_undo.instanceAttach(map);
  }
  void instanceDetach(MapFile* map)
  {
    m_undo.instanceDetach(map);
  }
};

namespace std
{
  /// \brief Swaps the values of \p self and \p other.
  /// Overloads std::swap.
  inline void swap(TraversableNodeSet& self, TraversableNodeSet& other)
  {
    self.swap(other);
  }
}

#endif
