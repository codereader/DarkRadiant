#include "TraversableNodeSet.h"

#include "debugging/debugging.h"
#include <algorithm>
#include "LayerValidityCheckWalker.h"
#include "BasicUndoMemento.h"
#include "mapfile.h"
#include "Node.h"

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

// Adds all visited nodes to the public list "nodes"
class CollectNodesFunctor :
	public ObserverFunctor
{
private:
	TraversableNodeSet::NodeList& _target;

public:
	CollectNodesFunctor(TraversableNodeSet::NodeList& target) :
		_target(target)
	{}

	void operator() (Node& owner, const INodePtr& node)
	{
		_target.push_back(node);
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
	Node* _owner;
	ObserverFunctor& _functor;
public:
	typedef std::output_iterator_tag iterator_category;
	typedef void difference_type;
	typedef void value_type;
	typedef void pointer;
	typedef void reference;

	ObserverOutputIterator(Node& owner, ObserverFunctor& functor) :
		_owner(&owner),
		_functor(functor)
	{}

	// This function is invoked by the std::set_difference algorithm
	ObserverOutputIterator& operator=(const INodePtr& node)
	{
		// Pass the call to the functor
		_functor(*_owner, node);
		return *this;
	}

	// Assignment operator, as needed by VC++ 2010's STL implementation
	ObserverOutputIterator& operator=(const ObserverOutputIterator& other)
	{
		// Pass the call to the functor
		_owner = other._owner;
		_functor = other._functor;
		return *this;
	}

	ObserverOutputIterator& operator*() { return *this; }
	ObserverOutputIterator& operator++() { return *this; }
	ObserverOutputIterator& operator++(int) { return *this; }
};

typedef undo::BasicUndoMemento<TraversableNodeSet::NodeList> UndoListMemento;

// Default constructor, creates an empty set
TraversableNodeSet::TraversableNodeSet(Node& owner) :
	_owner(owner),
	_undoStateSaver(NULL),
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
		child->traverse(visitor);
	}
}

bool TraversableNodeSet::foreachNode(const INode::VisitorFunc& functor) const
{
	for (NodeList::const_iterator i = _children.begin(); i != _children.end();)
	{
		const scene::INodePtr& child = *(i++); // readability shortcut

		// First, invoke the functor with this child node, 
		// stopping traversal if the functor returns false
		if (!functor(child))
		{
			return false;
		}

		// Pass the functor down the line, respecting the result
		if (!child->foreachNode(functor))
		{
			return false;
		}
	}

	return true; // all nodes passed
}

bool TraversableNodeSet::empty() const
{
	return _children.empty();
}

void TraversableNodeSet::instanceAttach(MapFile* map)
{
	_map = map;
	_undoStateSaver = GlobalUndoSystem().getStateSaver(*this);
}

void TraversableNodeSet::instanceDetach(MapFile* map)
{
	_map = NULL;
    _undoStateSaver = NULL;
	GlobalUndoSystem().releaseStateSaver(*this);
}

void TraversableNodeSet::undoSave()
{
    if (_map != NULL)
	{
		_map->changed();
	}

    if (_undoStateSaver != NULL)
	{
		_undoStateSaver->save(*this);
	}
}

IUndoMementoPtr TraversableNodeSet::exportState() const
{
	// Copy the current list of children and return the UndoMemento
	return IUndoMementoPtr(new UndoListMemento(_children));
}

void TraversableNodeSet::importState(const IUndoMementoPtr& state)
{
	undoSave();

	// Import the child set from the state
	const NodeList& other = std::static_pointer_cast<UndoListMemento>(state)->data();

	// Copy the current container into a temporary one for later comparison
	std::vector<INodePtr> before_sorted(_children.begin(), _children.end());
	std::vector<INodePtr> after_sorted(other.begin(), other.end());

	// greebo: Now sort these, the set_difference algorithm requires the sets to be sorted
	std::sort(before_sorted.begin(), before_sorted.end());
	std::sort(after_sorted.begin(), after_sorted.end());

	// Import the state, overwriting the current set
	_children = other;

	// Now, handle the difference of <before> and <after>
	// The owning node needs to know about all nodes which are removed in <after>, these are
	// instantly removed from the scenegraph
	ObserverEraseFunctor eraseFunctor;

	// greebo: Now find all the nodes that exist in <_children>, but not in <other> and
	// call the EraseFunctor for each of them (the iterator calls onChildRemoved() on the owning node).
	std::set_difference(
		before_sorted.begin(), before_sorted.end(),
		after_sorted.begin(), after_sorted.end(),
		ObserverOutputIterator(_owner, eraseFunctor)
	);

	// A special treatment is necessary for insertions of new nodes, as calling onChildAdded
	// right away might lead to double-insertions into the scenegraph (in case the same node
	// has not been removed from another node yet - a race condition during undo).
	// Therefore, collect all nodes that need to be added and process them in postUndo/postRedo.
	CollectNodesFunctor collectFunctor(_undoInsertBuffer);

	// greebo: Next step is to find all nodes existing in <other>, but not in <_children>,
	// these have to be added, that's why the onChildAdded() method is called for each of them
	std::set_difference(
		after_sorted.begin(), after_sorted.end(),
		before_sorted.begin(), before_sorted.end(),
		ObserverOutputIterator(_owner, collectFunctor)
	);

	if (!_undoInsertBuffer.empty())
	{
		// Register to get notified when the undo operation is complete
		GlobalUndoSystem().addObserver(this);
	}
}

void TraversableNodeSet::postUndo()
{
	processInsertBuffer();
	GlobalUndoSystem().removeObserver(this);
}

void TraversableNodeSet::postRedo()
{
	processInsertBuffer();
	GlobalUndoSystem().removeObserver(this);
}

void TraversableNodeSet::processInsertBuffer()
{
	for (NodeList::const_iterator i = _undoInsertBuffer.begin();
		 i != _undoInsertBuffer.end(); ++i)
	{
		_owner.onChildAdded(*i);

		LayerValidityCheckWalker::ProcessNode(*i);
	}

	// Clear the buffer after this operation
	_undoInsertBuffer.clear();
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

void TraversableNodeSet::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	for (NodeList::iterator i = _children.begin(); i != _children.end(); ++i)
	{
		(*i)->setRenderSystem(renderSystem);
	}
}

} // namespace scene
