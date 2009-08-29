#ifndef ITRAVERSABLE_H_
#define ITRAVERSABLE_H_

#include "inode.h"

namespace scene {

/** greebo: A Traversable is basically an element which can
 * 			have children and provides some basic methods
 * 			to insert/erase and traverse them.
 * 
 * 			Currently, the standard implementation is TraversableNodeSet.
 */
class Traversable
{
public:
	class Observer
	{
	public:
		/// \brief Called when a node is added to the container.
		virtual void onTraversableInsert(const INodePtr& node) = 0;
		/// \brief Called when a node is removed from the container.
		virtual void onTraversableErase(const INodePtr& node) = 0;
	};

	/// \brief Adds a node to the container.
	virtual void insert(INodePtr node) = 0;
	
	/// \brief Removes a node from the container.
	virtual void erase(INodePtr node) = 0;
    
	/**
	 * greebo: Traverses the children of this container, recursively.
	 * 
	 * Note: the Traversable itself is not visited, use Node_traverseSubgraph() 
	 * for this purpose.
	 */
	virtual void traverse(NodeVisitor& walker) const = 0;
    
	/// \brief Returns true if the container contains no nodes.
	virtual bool empty() const = 0;
};
typedef boost::shared_ptr<Traversable> TraversablePtr;

} // namespace scene

#endif /*ITRAVERSABLE_H_*/
