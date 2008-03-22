#include "RenderableTargetInstances.h"

#include "TargetableNode.h"

namespace entity {

// Add a TargetableInstance to this set
void RenderableTargetInstances::attach(TargetableNode& node) {
	ASSERT_MESSAGE(_nodes.find(&node) == _nodes.end(), "cannot attach instance");
	_nodes.insert(&node);
}

void RenderableTargetInstances::detach(TargetableNode& node) {
	ASSERT_MESSAGE(_nodes.find(&node) != _nodes.end(), "cannot detach instance");
	_nodes.erase(&node);
}

void RenderableTargetInstances::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	for (TargetableNodes::const_iterator i = _nodes.begin(); 
		 i != _nodes.end(); ++i)
	{
		(*i)->render(renderer, volume);
	}
}

void RenderableTargetInstances::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	renderSolid(renderer, volume);
}

RenderableTargetInstances& RenderableTargetInstances::Instance() {
	static RenderableTargetInstances _instances;
	return _instances;
}

} // namespace entity
