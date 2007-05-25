#ifndef INODE_H_
#define INODE_H_

#include <boost/shared_ptr.hpp>

namespace scene {

/** greebo: Abstract definition of a Node, a basic element
 * 			of the scenegraph.
 */
class INode
{
public:
	/** greebo: Returns true, if the node is the root element
	 * 			of the scenegraph.
	 */
	virtual bool isRoot() const = 0;
	
	/** greebo: Currently empty implementations of the layer
	 * 			accessor methods.
	 */
	virtual unsigned int getLayerFlags() { return 0; }
	virtual void addToLayer(const unsigned int& layer) {}
	virtual void removeFromLayer(const unsigned int& layer) {}
	
	/** greebo: State bit accessor methods. This enables/disables
	 * 			the bit of the state flag (e.g. hidden, excluded)
	 */
	virtual void enable(unsigned int state) = 0;
	virtual void disable(unsigned int state) = 0;

	/** greebo: Returns true, if the node is not hidden by 
	 * 			exclusion, filtering or anything else.
	 */
	virtual bool visible() const = 0;
	
	/** greebo: Returns true, if the node is excluded (eExcluded flag set)
	 */
	virtual bool excluded() const = 0;
};

typedef boost::shared_ptr<INode> INodePtr;

} // namespace scene

#endif /*INODE_H_*/
