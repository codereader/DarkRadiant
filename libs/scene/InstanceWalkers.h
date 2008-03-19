#ifndef SCENE_INSTANCE_WALKERS_H_
#define SCENE_INSTANCE_WALKERS_H_

#include "ipath.h"

namespace scene {

/** greebo: This Walker instantiates the visited nodes (the ones
 * 			that are Instantiable, that is). The whole subgraph is
 * 			traversed and GlobalSceneGraph().insert() is called on 
 *          the according nodes.
 */
class InstanceSubgraphWalker : 
	public scene::NodeVisitor
{
	scene::Path _path;
public:
	InstanceSubgraphWalker(const scene::Path& path);

	virtual bool pre(const scene::INodePtr& node);
	virtual void post(const scene::INodePtr& node);
};

/** greebo: This Walker un-instantiates the visited nodes (the ones
 * 			that are Instantiable, that is). The whole subgraph is
 * 			traversed and erase() is called on the according nodes.
 */
class UninstanceSubgraphWalker : 
	public scene::NodeVisitor
{
	scene::Path _path;
public:
	UninstanceSubgraphWalker(const scene::Path& parent);
	
	virtual bool pre(const scene::INodePtr& node);
	virtual void post(const scene::INodePtr& node);
};

} // namespace scene

#endif /* SCENE_INSTANCE_WALKERS_H_ */
