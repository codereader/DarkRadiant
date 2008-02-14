#include "RenderableTargetInstances.h"

#include "TargetableInstance.h"

namespace entity {

// Add a TargetableInstance to this set
void RenderableTargetInstances::attach(TargetableInstance& instance) {
	ASSERT_MESSAGE(_instances.find(&instance) == _instances.end(), "cannot attach instance");
	_instances.insert(&instance);
}

void RenderableTargetInstances::detach(TargetableInstance& instance) {
	ASSERT_MESSAGE(_instances.find(&instance) != _instances.end(), "cannot detach instance");
	_instances.erase(&instance);
}

void RenderableTargetInstances::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	for (TargetableInstances::const_iterator i = _instances.begin(); 
		 i != _instances.end(); ++i)
	{
		TargetableInstance* instance = *i;

		if (instance->path().top()->visible()) {
			instance->render(renderer, volume);
		}
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
