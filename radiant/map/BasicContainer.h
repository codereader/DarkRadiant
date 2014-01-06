#pragma once

#include "scene/Node.h"
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

	Type getNodeType() const
	{
		return Type::Unknown;
	}

	// Renderable implementation (empty)
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
	{}

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
	{}

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		// Nothing to render
	}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}
};
typedef boost::shared_ptr<BasicContainer> BasicContainerPtr;

} // namespace map
