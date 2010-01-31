#ifndef SCENE_TRAVERSABLENODESET_H_
#define SCENE_TRAVERSABLENODESET_H_

#include "inode.h"
#include "iundo.h"
#include <list>
#include <boost/noncopyable.hpp>

class MapFile;

namespace scene
{

class Node;

/// \brief A sequence of node references which notifies an observer of inserts and deletions, and uses the global undo system to provide undo for modifications.

/** greebo: This is the container holding all the child nodes of a scene::Node.
 * 
 * The TraversableNodeSet is also reporting any changes to the UndoSystem, that's what the
 * instanceAttach() methods are for. An UndoMemento is submitted to the UndoSystem as soon
 * as any child nodes are removed or inserted. When the user hits Undo, the UndoSystem sends back
 * the memento and asks the TraversableNodeSet to overwrite its current children with the saved state.  
 */
class TraversableNodeSet :
	public Undoable,
	public boost::noncopyable
{
	typedef std::list<INodePtr> NodeList;
	NodeList _children;
	
	// The owning node which gets notified upon insertion/deletion of child nodes 
	Node& _owner;

	UndoObserver* _undoObserver;
	MapFile* _map;

public:
	// Default constructor, creates an empty set
	TraversableNodeSet(Node& owner);
	
	// Destructor
	~TraversableNodeSet();
	
	/** 
	 * greebo: This inserts a child node, saves the Undo state and notifies the owning node.
	 */ 
	void insert(const INodePtr& node);
	
	/** 
	 * greebo: This removes the node from the local set, saves the UndoMemento and notifies the owning node.
	 */
	void erase(const INodePtr& node);

	/** 
	 * Removes all nodes from this container. Notifies the owning node for each deleted node.
	 */
	void clear();
	
	/** 
	 * greebo: This visits all the child nodes using the given visitor scene::NodeVisitor.
	 */
	void traverse(NodeVisitor& visitor) const;
	
	/** 
	 * greebo: Returns TRUE if this NodeSet is empty.
	 */
	bool empty() const;

	void instanceAttach(MapFile* map);	
	void instanceDetach(MapFile* map);

	// Undoable implementation
	UndoMemento* exportState() const;
	void importState(const UndoMemento* state);

private:
	// Sends the current state to the undosystem
	void undoSave();

	void notifyInsertAll();	
	void notifyEraseAll();
	
	/** 
	 * greebo: This calls insertChild() and eraseChild() only for the nodes that exclusively exist 
	 * in one of the input sets <before> and <after>. All nodes existing only in <other> will 
	 * trigger a call insertChild() on the owning node.
	 * 
	 * 	Note: this does not change the input sets, it calls the Observer only. 
	 */
	void notifyDifferent(const NodeList& before, const NodeList& other);
};

} // namespace scene

#endif /* SCENE_TRAVERSABLENODESET_H_ */
