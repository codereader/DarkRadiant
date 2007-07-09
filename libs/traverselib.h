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

#include <list>
#include <algorithm>

// An ObserverFunctor does something with the given <observer> and the given <node> 
struct ObserverFunctor {
	virtual void operator() (scene::Traversable::Observer& observer, scene::INodePtr node) = 0;
};

// This calls eraseChild() on the given <observer>
struct ObserverEraseFunctor :
	public ObserverFunctor
{
	virtual void operator() (scene::Traversable::Observer& _observer, scene::INodePtr node) {
		_observer.eraseChild(node);
	}
};

// This calls insertChild() on the given <observer>
struct ObserverInsertFunctor :
	public ObserverFunctor
{
	virtual void operator() (scene::Traversable::Observer& _observer, scene::INodePtr node) {
		_observer.insertChild(node);
	}
};

/** greebo: This iterator is required by the std::set_difference algorithm and 
 * 			is used to call	scene::Traversable::Observer::insertChild() or 
 * 			eraseChild() as soon as	the assignment operator is invoked by 
 * 			the set_difference algorithm.
 * 
 * Note: The operator++ is apparently necessary, but is not doing anything, as this iterator
 * 		 is only "fake" and is only used to trigger the observer call. 
 */
class ObserverOutputIterator 
{
protected:
	scene::Traversable::Observer* _observer;
	ObserverFunctor& _functor;
public:
	typedef std::output_iterator_tag iterator_category;
	typedef void difference_type;
	typedef void value_type;
	typedef void pointer;
	typedef void reference;

	ObserverOutputIterator(scene::Traversable::Observer* observer, ObserverFunctor& functor) : 
		_observer(observer),
		_functor(functor)
	{}
	
	// This function is invoked by the std::set_difference algorithm
	ObserverOutputIterator& operator=(const scene::INodePtr node) {
		// Pass the call to the functor
		_functor(*_observer, node); 
		return *this;
	}
	
	ObserverOutputIterator& operator*() { return *this; }
	ObserverOutputIterator& operator++() { return *this; }
	ObserverOutputIterator& operator++(int) { return *this; }
};

/// \brief A sequence of node references which notifies an observer of inserts and deletions, and uses the global undo system to provide undo for modifications.

/** greebo: This is the standard implementation of scene::Traversable. All container nodes
 * 			(this is Doom3GroupNode, EClassModelNode and RootNode at the time of writing) derive
 * 			from this class.
 * 
 * 			This class supports one scene::Traversable::Observer to be attached, that gets notified
 * 			upon insertions and deletions of any child nodes.
 * 
 * 			The TraversableNodeSet is also reporting any changes to the UndoSystem, that's what the
 * 			instanceAttach() methods are for. The UndoableObject is submitted to the UndoSystem as soon
 * 			as any child nodes are removed or inserted. When the user hits Undo, the contained UndoableObject
 * 			uses the assignment operator to overwrite this class with the saved state.  
 */
class TraversableNodeSet : 
	public scene::Traversable
{
	typedef std::list<scene::INodePtr> NodeList;
	NodeList _children;
	
	// The undoable object, which overwrites the content of this class on demand
	UndoableObject<TraversableNodeSet> _undo;
	
	// The observer which gets notified upon insertion/deletion of child nodes 
	Observer* _observer;

public:
	// Default constructor, creates an empty set
	TraversableNodeSet() : 
		_undo(*this), 
		_observer(NULL)
	{}
	
	// Copy Constructor, copies all the nodes from the other set, but not the Observer
	TraversableNodeSet(const TraversableNodeSet& other) : 
		scene::Traversable(other), 
		_undo(*this), 
		_observer(NULL)
	{
		_children = other._children;
		notifyInsertAll();
	}
	
	// Destructor
	~TraversableNodeSet() {
		notifyEraseAll();
	}
	
	/** greebo: This assignment operator is invoked by the UndoableObject
	 * 			as soon as the user hits Undo or Redo.
	 */
	TraversableNodeSet& operator=(const TraversableNodeSet& other) {
		// Notify the observer on the difference of the two sets
		if (_observer != NULL) {
			notifyDifferent(other._children);
		}
		
		// Copy the child nodes
		_children = other._children;
		
		return *this;
	}
	
	/** greebo: Attaches an Observer to this set, which gets immediately notified
	 * 			about all the existing child nodes.
	 */
	void attach(Observer* observer) {
		ASSERT_MESSAGE(_observer == 0, "TraversableNodeSet::attach: observer cannot be attached");
		_observer = observer;
		notifyInsertAll();
	}
	
	/** greebo: Detaches the Observer from this set. This also triggers an erase() call
	 * 			for all the existing child nodes on the observer.
	 */
	void detach(Observer* observer) {
		ASSERT_MESSAGE(_observer == observer, "TraversableNodeSet::detach: observer cannot be detached");
		notifyEraseAll();
		_observer = NULL;
	}
	
	/** greebo: scene::Traversable implementation, this inserts a child node, saves the Undo state
	 * 			and notifies the observer (if there is one)
	 */ 
	void insert(scene::INodePtr node) {
		// Submit the UndoMemento to the UndoSystem
		_undo.save();

		//ASSERT_MESSAGE(_children.find(node) == _children.end(), "TraversableNodeSet::insert - element already exists");

		// Insert the child node at the end of the list
		_children.push_back(node);

		// Notify the observer (note: this usually triggers instantiation of the node)
		if (_observer != NULL) {
			_observer->insertChild(node);
		}
	}
	
	/** greebo: scene::Traversable implementation. This removes the node from the local set,
	 * 			saves the UndoMemento and notifies the observer.
	 */
	void erase(scene::INodePtr node) {
		ASSERT_MESSAGE(&node != 0, "TraversableNodeSet::erase: sanity check failed");
		_undo.save();

		//ASSERT_MESSAGE(_children.find(node) != _children.end(), "TraversableNodeSet::erase - failed to find element");

		// Notify the Observer before actually removing the node 
		if (_observer != NULL) {
			_observer->eraseChild(node);
		}

		// Lookup the node and remove it from the list
		NodeList::iterator i = std::find(_children.begin(), _children.end(), node);
	    if (i != _children.end()) {
	        _children.erase(i);
	    }
	}
	
	/// \brief \copydoc scene::Traversable::traverse()
	/** greebo: scene::Traversable implementation. This visits all the child nodes
	 * 			using the given visitor scene::Traversable::Walker
	 */
	void traverse(const Walker& walker) {
		NodeList::iterator i = _children.begin();
		
		while (i != _children.end()) {
			// post-increment the iterator
			Node_traverseSubgraph(*i++, walker);
			
			// Note: the Walker can safely remove the current node from
			// this container without invalidating the iterator
		}
	}
	
	/** greebo: scene::Traversable implementation. Returns TRUE if this NodeSet is empty.
	 */
	bool empty() const {
		return _children.empty();
	}

	void instanceAttach(MapFile* map) {
		_undo.instanceAttach(map);
	}
	
	void instanceDetach(MapFile* map) {
		_undo.instanceDetach(map);
	}

private:
	void notifyInsertAll() {
		if (_observer != NULL) {
			for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i) {
				_observer->insertChild(*i);
			}
		}
	}
	
	void notifyEraseAll() {
		if (_observer != NULL) {
			for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i) {
				_observer->eraseChild(*i);
			}
		}
	}
	
	/// \brief Calls \p observer->\c insert for each node that exists only in \p other and \p observer->\c erase for each node that exists only in \p self

	/** greebo: This calls insertChild() and eraseChild() only for the nodes that exclusively exist 
	 * 			in one of the input sets <self> and <other>. All nodes existing only in <other> will 
	 * 			trigger a call insertChild() on the observer.
	 * 
	 * 	Note: this does not change the input sets, it calls the Observer only. 
	 */
	void notifyDifferent(const NodeList& other) {
		// greebo: Copy the values from the source sets <_children> and <other> into temporary vectors
		std::vector<scene::INodePtr> sorted(_children.begin(), _children.end());
		std::vector<scene::INodePtr> other_sorted(other.begin(), other.end());
	
		// greebo: Now sort these, the set_difference algorithm requires the sets to be sorted
		std::sort(sorted.begin(), sorted.end());
		std::sort(other_sorted.begin(), other_sorted.end());
	
		ObserverEraseFunctor eraseFunctor;
		ObserverInsertFunctor insertFunctor;
	
		// greebo: Now find all the nodes that exist in <_children>, but not in <other> and 
		// call the EraseFunctor for each of them (the iterator calls eraseChild() on the given observer). 
		std::set_difference(
			sorted.begin(), sorted.end(), 
			other_sorted.begin(), other_sorted.end(), 
			ObserverOutputIterator(_observer, eraseFunctor)
		);
		
		// greebo: Next step is to find all nodes existing in <other>, but not in <_children>, 
		// these have to be added, that's why the insertChild() method is called for each of them  
		std::set_difference(
			other_sorted.begin(), other_sorted.end(), 
			sorted.begin(), sorted.end(), 
			ObserverOutputIterator(_observer, insertFunctor)
		);
	}
};

#endif
