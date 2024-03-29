#pragma once

#include "inode.h"
#include "iundo.h"
#include <list>
#include "util/Noncopyable.h"

namespace scene
{

class Node;

/// \brief A sequence of node references which notifies an observer of inserts and deletions, and uses the global undo system to provide undo for modifications.

/** greebo: This is the container holding all the child nodes of a scene::Node.
 *
 * The TraversableNodeSet is also reporting any changes to the UndoSystem, that's what the
 * onInsertIntoScene(root) methods are for. An UndoMemento is submitted to the UndoSystem as soon
 * as any child nodes are removed or inserted. When the user hits Undo, the UndoSystem sends back
 * the memento and asks the TraversableNodeSet to overwrite its current children with the saved state.
 */
class TraversableNodeSet final :
	public IUndoable,
	public util::Noncopyable,
	public sigc::trackable
{
public:
	typedef std::list<INodePtr> NodeList;

private:
	NodeList _children;

	// The owning node which gets notified upon insertion/deletion of child nodes
	Node& _owner;

	IUndoStateSaver* _undoStateSaver;

	// A list collecting nodes for insertion in postUndo/postRedo
	NodeList _undoInsertBuffer;

public:
	// Default constructor, creates an empty set
	TraversableNodeSet(Node& owner);

	// Destructor
	~TraversableNodeSet();

	/**
	 * greebo: This inserts a child node at the end of the list, 
	 * saves the Undo state and notifies the owning node.
	 */
	void append(const INodePtr& node);

	/**
	* greebo: This inserts a child node at the front of the list,
	* saves the Undo state and notifies the owning node.
	*/
	void prepend(const INodePtr& node);

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

	void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

	// Undoable implementation
	IUndoMementoPtr exportState() const override;
	void importState(const IUndoMementoPtr& state) override;
    void onOperationRestored() override;

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
