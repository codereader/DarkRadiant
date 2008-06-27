#include "TraversableNodeSet.h"

#include "debugging/debugging.h"
#include <algorithm>
#include "scenelib.h"

// An ObserverFunctor does something with the given <observer> and the given <node> 
struct ObserverFunctor {
	virtual void operator() (scene::Traversable::Observer& observer, scene::INodePtr node) = 0;
};

// This calls onTraversableErase() on the given <observer>
struct ObserverEraseFunctor :
	public ObserverFunctor
{
	virtual void operator() (scene::Traversable::Observer& _observer, scene::INodePtr node) {
		_observer.onTraversableErase(node);
	}
};

// This calls onTraversableInsert() on the given <observer>
struct ObserverInsertFunctor :
	public ObserverFunctor
{
	virtual void operator() (scene::Traversable::Observer& _observer, scene::INodePtr node) {
		_observer.onTraversableInsert(node);
	}
};

/** greebo: This iterator is required by the std::set_difference algorithm and 
 * 			is used to call	scene::Traversable::Observer::onTraversableInsert() or 
 * 			onTraversableErase() as soon as	the assignment operator is invoked by 
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

// Default constructor, creates an empty set
TraversableNodeSet::TraversableNodeSet() : 
	_undo(*this), 
	_observer(NULL)
{}

// Copy Constructor, copies all the nodes from the other set, but not the Observer
TraversableNodeSet::TraversableNodeSet(const TraversableNodeSet& other) : 
	scene::Traversable(other), 
	_undo(*this), 
	_observer(NULL)
{
	_children = other._children;
	notifyInsertAll();
}

// Destructor
TraversableNodeSet::~TraversableNodeSet() {
	notifyEraseAll();
}

/** greebo: This assignment operator is invoked by the UndoableObject
 * 			as soon as the user hits Undo or Redo.
 */
TraversableNodeSet& TraversableNodeSet::operator=(const TraversableNodeSet& other) {
	// Copy the local container into a temporary one for later comparison
	NodeList before(_children);

	// Copy the other container over
	_children = other._children;

	// Notify the observer on the difference of the two sets
	// Use the previously saved container for comparison
	if (_observer != NULL) {
		notifyDifferent(before, other._children);
	}
	
	return *this;
}

/** greebo: Attaches an Observer to this set, which gets immediately notified
 * 			about all the existing child nodes.
 */
void TraversableNodeSet::attach(Observer* observer) {
	ASSERT_MESSAGE(_observer == 0, "TraversableNodeSet::attach: observer cannot be attached");
	_observer = observer;
	notifyInsertAll();
}

/** greebo: Detaches the Observer from this set. This also triggers an erase() call
 * 			for all the existing child nodes on the observer.
 */
void TraversableNodeSet::detach(Observer* observer) {
	ASSERT_MESSAGE(_observer == observer, "TraversableNodeSet::detach: observer cannot be detached");
	notifyEraseAll();
	_observer = NULL;
}

/** greebo: scene::Traversable implementation, this inserts a child node, saves the Undo state
 * 			and notifies the observer (if there is one)
 */ 
void TraversableNodeSet::insert(scene::INodePtr node) {
	// Submit the UndoMemento to the UndoSystem
	_undo.save();

	//ASSERT_MESSAGE(_children.find(node) == _children.end(), "TraversableNodeSet::insert - element already exists");

	// Insert the child node at the end of the list
	_children.push_back(node);

	// Notify the observer (note: this usually triggers instantiation of the node)
	if (_observer != NULL) {
		_observer->onTraversableInsert(node);
	}
}

/** greebo: scene::Traversable implementation. This removes the node from the local set,
 * 			saves the UndoMemento and notifies the observer.
 */
void TraversableNodeSet::erase(scene::INodePtr node) {
	ASSERT_MESSAGE(&node != 0, "TraversableNodeSet::erase: sanity check failed");
	_undo.save();

	//ASSERT_MESSAGE(_children.find(node) != _children.end(), "TraversableNodeSet::erase - failed to find element");

	// Notify the Observer before actually removing the node 
	if (_observer != NULL) {
		_observer->onTraversableErase(node);
	}

	// Lookup the node and remove it from the list
	NodeList::iterator i = std::find(_children.begin(), _children.end(), node);
    if (i != _children.end()) {
        _children.erase(i);
    }
}

void TraversableNodeSet::clear() {
	// Remove each child until empty
	while (!_children.empty()) {
		erase(*_children.begin());
	}
}

void TraversableNodeSet::traverse(scene::NodeVisitor& visitor) {
	for (NodeList::const_iterator i = _children.begin();
		 i != _children.end();)
	{
		const scene::INodePtr& child = *(i++); // readability shortcut
		// Traverse the child using the visitor
		Node_traverseSubgraph(child, visitor);
	}
}

/** greebo: scene::Traversable implementation. Returns TRUE if this NodeSet is empty.
 */
bool TraversableNodeSet::empty() const {
	return _children.empty();
}

void TraversableNodeSet::instanceAttach(MapFile* map) {
	_undo.instanceAttach(map);
}

void TraversableNodeSet::instanceDetach(MapFile* map) {
	_undo.instanceDetach(map);
}

void TraversableNodeSet::notifyInsertAll() {
	if (_observer != NULL) {
		for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i) {
			_observer->onTraversableInsert(*i);
		}
	}
}

void TraversableNodeSet::notifyEraseAll() {
	if (_observer != NULL) {
		for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i) {
			_observer->onTraversableErase(*i);
		}
	}
}

/// \brief Calls \p observer->\c insert for each node that exists only in \p other and \p observer->\c erase for each node that exists only in \p self

/** greebo: This calls onTraversableInsert() and onTraversableErase() only for the nodes that exclusively exist 
 * 			in one of the input sets <before> and <after>. All nodes existing only in <other> will 
 * 			trigger a call onTraversableInsert() on the observer.
 * 
 * 	Note: this does not change the input sets, it calls the Observer only. 
 */
void TraversableNodeSet::notifyDifferent(const NodeList& before, const NodeList& after) {
	// greebo: Copy the values from the source sets <_children> and <other> into temporary vectors
	std::vector<scene::INodePtr> before_sorted(before.begin(), before.end());
	std::vector<scene::INodePtr> after_sorted(after.begin(), after.end());

	// greebo: Now sort these, the set_difference algorithm requires the sets to be sorted
	std::sort(before_sorted.begin(), before_sorted.end());
	std::sort(after_sorted.begin(), after_sorted.end());

	ObserverEraseFunctor eraseFunctor;
	ObserverInsertFunctor insertFunctor;

	// greebo: Now find all the nodes that exist in <_children>, but not in <other> and 
	// call the EraseFunctor for each of them (the iterator calls onTraversableErase() on the given observer). 
	std::set_difference(
		before_sorted.begin(), before_sorted.end(), 
		after_sorted.begin(), after_sorted.end(), 
		ObserverOutputIterator(_observer, eraseFunctor)
	);
	
	// greebo: Next step is to find all nodes existing in <other>, but not in <_children>, 
	// these have to be added, that's why the onTraversableInsert() method is called for each of them  
	std::set_difference(
		after_sorted.begin(), after_sorted.end(), 
		before_sorted.begin(), before_sorted.end(), 
		ObserverOutputIterator(_observer, insertFunctor)
	);
}
