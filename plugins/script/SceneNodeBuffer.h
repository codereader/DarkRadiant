#ifndef _SCENE_NODE_BUFFER_H_
#define _SCENE_NODE_BUFFER_H_

#include <vector>
#include "inode.h"

namespace script
{

// Singleton vector carrying scene::INodePtrs
// Used to prevent auto-destruction on node creation
class SceneNodeBuffer :
	public std::vector<scene::INodePtr>
{
public:
	static SceneNodeBuffer& Instance();
};

} // namespace script

#endif /* _SCENE_NODE_BUFFER_H_ */
