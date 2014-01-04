#pragma once

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
	public IUndoable,
	public boost::noncopyable,
	public UndoSystem::Observer
{
public:
	typedef std::list<INodePtr> NodeList;

private:
	NodeList _children;

	// The owning node which gets notified upon insertion/deletion of child nodes
	Node& _owner;

	IUndoStateSaver* _undoStateSaver;
	MapFile* _map;

	// A list collecting nodes for insertion in postUndo/postRedo
	NodeList _undoInsertBuffer;

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
	 * Visits each contained node with the given functor, recursively, depth-first.
	 */
	bool foreachNode(const INode::VisitorFunc& functor) const;

	/**
	 * greebo: Returns TRUE if this NodeSet is empty.
	 */
	bool empty() const;

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	// Undoable implementation
	IUndoMementoPtr exportState() const;
	void importState(const IUndoMementoPtr& state);

	// UndoSystem::Observer implementation
	void postUndo();
	void postRedo();

	void setRenderSystem(const RenderSystemPtr& renderSystem);

private:
	// Sends the current state to the undosystem
	void undoSave();

	// Calls the owning node for each node in the undo insert buffer,
	// this is called right after an undo operation
	void processInsertBuffer();

	void notifyInsertAll();
	void notifyEraseAll();
};

} // namespace scene
