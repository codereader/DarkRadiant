#ifndef SCENE_TRAVERSABLENODESET_H_
#define SCENE_TRAVERSABLENODESET_H_

#include "inode.h"
#include <list>
#include "itraversable.h"
#include "UndoableObject.h"

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
	TraversableNodeSet();
	
	// Copy Constructor, copies all the nodes from the other set, but not the Observer
	TraversableNodeSet(const TraversableNodeSet& other);
	
	// Destructor
	~TraversableNodeSet();
	
	/** greebo: This assignment operator is invoked by the UndoableObject
	 * 			as soon as the user hits Undo or Redo.
	 */
	TraversableNodeSet& operator=(const TraversableNodeSet& other);
	
	/** greebo: Attaches an Observer to this set, which gets immediately notified
	 * 			about all the existing child nodes.
	 */
	void attach(Observer* observer);
	
	/** greebo: Detaches the Observer from this set. This also triggers an erase() call
	 * 			for all the existing child nodes on the observer.
	 */
	void detach(Observer* observer);
	
	/** greebo: scene::Traversable implementation, this inserts a child node, saves the Undo state
	 * 			and notifies the observer (if there is one)
	 */ 
	void insert(scene::INodePtr node);
	
	/** greebo: scene::Traversable implementation. This removes the node from the local set,
	 * 			saves the UndoMemento and notifies the observer.
	 */
	void erase(scene::INodePtr node);

	/** 
	 * Removes all nodes from this container. Notifies the observers.
	 */
	void clear();
	
	/** greebo: scene::Traversable implementation. This visits all the child nodes
	 * 			using the given visitor scene::NodeVisitor.
	 */
	void traverse(scene::NodeVisitor& visitor);
	
	/** greebo: scene::Traversable implementation. Returns TRUE if this NodeSet is empty.
	 */
	bool empty() const;

	void instanceAttach(MapFile* map);	
	void instanceDetach(MapFile* map);

private:
	void notifyInsertAll();	
	void notifyEraseAll();
	
	/// \brief Calls \p observer->\c insert for each node that exists only in \p other and \p observer->\c erase for each node that exists only in \p self

	/** greebo: This calls insertChild() and eraseChild() only for the nodes that exclusively exist 
	 * 			in one of the input sets <before> and <after>. All nodes existing only in <other> will 
	 * 			trigger a call insertChild() on the observer.
	 * 
	 * 	Note: this does not change the input sets, it calls the Observer only. 
	 */
	void notifyDifferent(const NodeList& before, const NodeList& other);
};

#endif /* SCENE_TRAVERSABLENODESET_H_ */
