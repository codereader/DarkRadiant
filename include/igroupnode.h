#ifndef IGROUPNODE_H_
#define IGROUPNODE_H_

#include "inode.h"

namespace scene {

/** greebo: This is used to identify group entities right before
 * and after map save/load.
 * 
 * It provides methods to add/substract the origin to/from 
 * their child primitives.
 */
class GroupNode {
public:
	/** greebo: This is called right before saving
	 * to move the child brushes of the Doom3Group
	 * according to its origin.
	 */
	virtual void addOriginToChildren() = 0;
	virtual void removeOriginFromChildren() = 0;
};
typedef boost::shared_ptr<GroupNode> GroupNodePtr;

} // namespace scene

/** greebo: Cast a node onto a GroupNode pointer
 * 
 * @returns: NULL, if failed, the pointer to the class otherwise.
 */
inline scene::GroupNodePtr Node_getGroupNode(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<scene::GroupNode>(node);
}

#endif /*IGROUPNODE_H_*/
