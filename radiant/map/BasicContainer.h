#pragma once

#include "scenelib.h"
#include "inamespace.h"

namespace map {

/**
 * greebo: This is a temporary container (node) used during map object import.
 * It possesses its own Namespace which all inserted child nodes get connected to.
 */
class BasicContainer :
	public scene::Node
{
private:
	AABB _emptyAABB;

public:
	const AABB& localAABB() const
	{
		return _emptyAABB;
	}

	// Renderable implementation (empty)
	virtual void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
	{}

	virtual void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
	{}
};
typedef boost::shared_ptr<BasicContainer> BasicContainerPtr;

} // namespace map
