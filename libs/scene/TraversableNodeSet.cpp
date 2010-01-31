#include "TraversableNodeSet.h"

#include "debugging/debugging.h"
#include <algorithm>
#include "scenelib.h"
#include "undolib.h"

namespace scene
{

// An ObserverFunctor does something with the given owner and a given child <node> 
struct ObserverFunctor {
    virtual ~ObserverFunctor() {}
	virtual void operator() (Node& owner, const INodePtr& node) = 0;
};

// This calls onChildRemoved() on the given <observer>
struct ObserverEraseFunctor :
	public ObserverFunctor
{
	void operator() (Node& owner, const INodePtr& node)
	{
		owner.onChildRemoved(node);
	}
};

// This calls onChildAdded() on owner for the given child node
struct ObserverInsertFunctor :
	public ObserverFunctor
{
	void operator() (Node& owner, const INodePtr& node)
	{
		owner.onChildAdded(node);
	}
};

/** greebo: This iterator is required by the std::set_difference algorithm and 
 * is used to call	owning node's onChildAdded() or onChildRemoved() 
 * as soon as the assignment operator is invoked by the set_difference algorithm.
 * 
 * Note: The operator++ is apparently necessary, but is not doing anything, as this iterator
 * is only "fake" and is only used to trigger the observer call. 
 */
class ObserverOutputIterator 
{
protected:
	Node& _owner;
	ObserverFunctor& _functor;
public:
	typedef std::output_iterator_tag iterator_category;
	typedef void difference_type;
	typedef void value_type;
	typedef void pointer;
	typedef void reference;

	ObserverOutputIterator(Node& owner, ObserverFunctor& functor) : 
		_owner(owner),
		_functor(functor)
	{}
	
	// This function is invoked by the std::set_difference algorithm
	ObserverOutputIterator& operator=(const INodePtr& node) {
		// Pass the call to the functor
		_functor(_owner, node); 
		return *this;
	}
	
	ObserverOutputIterator& operator*() { return *this; }
	ObserverOutputIterator& operator++() { return *this; }
	ObserverOutputIterator& operator++(int) { return *this; }
};

// Default constructor, creates an empty set
TraversableNodeSet::TraversableNodeSet(Node& owner) : 
	_owner(owner),
	_undoObserver(NULL),
	_map(NULL)
{}

// Destructor
TraversableNodeSet::~TraversableNodeSet()
{
	notifyEraseAll();
}

void TraversableNodeSet::insert(const INodePtr& node)
{
	// Submit the UndoMemento to the UndoSystem
	undoSave();

	// Insert the child node at the end of the list
	_children.push_back(node);

	// Notify the owner (note: this usually triggers instantiation of the node)
	_owner.onChildAdded(node);
}

void TraversableNodeSet::erase(const INodePtr& node) 
{
	undoSave();

	// Notify the Observer before actually removing the node 
	_owner.onChildRemoved(node);

	// Lookup the node and remove it from the list
	NodeList::iterator i = std::find(_children.begin(), _children.end(), node);

    if (i != _children.end())
	{
        _children.erase(i);
    }
}

void TraversableNodeSet::clear()
{
	undoSave();
	notifyEraseAll();
	_children.clear();
}

void TraversableNodeSet::traverse(NodeVisitor& visitor) const
{
	for (NodeList::const_iterator i = _children.begin();
		 i != _children.end();)
	{
		const scene::INodePtr& child = *(i++); // readability shortcut
		// Traverse the child using the visitor
		Node_traverseSubgraph(child, visitor);
	}
}

bool TraversableNodeSet::empty() const
{
	return _children.empty();
}

void TraversableNodeSet::instanceAttach(MapFile* map)
{
	_map = map;
    _undoObserver = GlobalUndoSystem().observer(this);
}

void TraversableNodeSet::instanceDetach(MapFile* map)
{
	_map = NULL;
    _undoObserver = NULL;
	GlobalUndoSystem().release(this);
}

void TraversableNodeSet::undoSave()
{
    if (_map != NULL)
	{
		_map->changed();
	}

    if (_undoObserver != NULL)
	{
		_undoObserver->save(this);
	}
}

UndoMemento* TraversableNodeSet::exportState() const
{
	// Copy the current list of children and return the UndoMemento
	return new BasicUndoMemento<NodeList>(_children);
}

void TraversableNodeSet::importState(const UndoMemento* state)
{
	undoSave();

	// Import the child set from the state
	const NodeList& other = static_cast<const BasicUndoMemento<NodeList>*>(state)->get();

	// Copy the current container into a temporary one for later comparison
	NodeList before(_children);

	// Import the state, overwriting the current set
	_children = other;

	// Notify the owner on the difference of the two sets
	// Use the previously saved container for comparison
	notifyDifferent(before, other);
}

void TraversableNodeSet::notifyInsertAll()
{
	for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		_owner.onChildAdded(*i);
	}
}

void TraversableNodeSet::notifyEraseAll()
{
	for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		_owner.onChildRemoved(*i);
	}
}

void TraversableNodeSet::notifyDifferent(const NodeList& before, const NodeList& after)
{
	// greebo: Copy the values from the source sets <_children> and <other> into temporary vectors
	std::vector<INodePtr> before_sorted(before.begin(), before.end());
	std::vector<INodePtr> after_sorted(after.begin(), after.end());

	// greebo: Now sort these, the set_difference algorithm requires the sets to be sorted
	std::sort(before_sorted.begin(), before_sorted.end());
	std::sort(after_sorted.begin(), after_sorted.end());

	ObserverEraseFunctor eraseFunctor;
	ObserverInsertFunctor insertFunctor;

	// greebo: Now find all the nodes that exist in <_children>, but not in <other> and 
	// call the EraseFunctor for each of them (the iterator calls onChildRemoved() on the given observer). 
	std::set_difference(
		before_sorted.begin(), before_sorted.end(), 
		after_sorted.begin(), after_sorted.end(), 
		ObserverOutputIterator(_owner, eraseFunctor)
	);
	
	// greebo: Next step is to find all nodes existing in <other>, but not in <_children>, 
	// these have to be added, that's why the onChildAdded() method is called for each of them  
	std::set_difference(
		after_sorted.begin(), after_sorted.end(), 
		before_sorted.begin(), before_sorted.end(), 
		ObserverOutputIterator(_owner, insertFunctor)
	);
}

} // namespace scene
