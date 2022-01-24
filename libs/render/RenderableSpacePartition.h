#pragma once

#include "ispacepartition.h"
#include "irender.h"
#include "irenderable.h"

#include "math/Matrix4.h"
#include "math/AABB.h"
#include "render/RenderableColouredBoundingBoxes.h"

namespace render
{

/**
 * greebo: This is a renderable helper object which can be used
 * to render any space partition system implementing the ISPacepartitionSystem interface.
 *
 * Instantiate such a class and pass the Shader and the SpacePartition system to the
 * setShader() and setSpacePartition() methods to enable rendering.
 *
 * This object can be directly attached to the GlobalRenderSystem().
 */
class RenderableSpacePartition :
	public Renderable
{
private:
	// The shader we're using
	ShaderPtr _shader;

	// The space partition to render
	scene::ISpacePartitionSystemPtr _spacePartition;

    std::vector<AABB> _spacePartitionNodes;
    std::vector<Vector4> _nodeColours;
    RenderableColouredBoundingBoxes _renderableBoxes;

public:
    RenderableSpacePartition() :
        _renderableBoxes(_spacePartitionNodes, _nodeColours)
    {}

	void setSpacePartition(const scene::ISpacePartitionSystemPtr& spacePartition)
	{
		_spacePartition = spacePartition;
	}

    void onPreRender(const VolumeTest& volume) override
    {
        if (!_spacePartition)
        {
            _renderableBoxes.clear();
            return;
        }

        // Accumulate the bounding boxes to render
        _spacePartitionNodes.clear();
        _nodeColours.clear();

        accumulateBoundingBoxes(_spacePartition->getRoot());

        // Update the renderable every frame
        _renderableBoxes.queueUpdate();
        _renderableBoxes.update(_shader);
    }

    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override
    {}

	void setRenderSystem(const RenderSystemPtr& renderSystem) override
	{
        _renderableBoxes.clear();

		if (renderSystem)
		{
			_shader = renderSystem->capture("[1 0 0]");
		}
		else
		{
			_shader.reset();
		}
	}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

private:
	void accumulateBoundingBoxes(const scene::ISPNodePtr& node)
	{
		const auto& members = node->getMembers();

		float shade = members.size() > 2 ? 1 : (members.size() > 0 ? 0.6f : 0);

        _nodeColours.emplace_back(shade, shade, shade, 1);

		AABB rb(node->getBounds());

		// Extend the renderbounds *slightly* so that the lines don't overlap
		rb.extents *= 1.02f;

        _spacePartitionNodes.push_back(rb);

		for (auto child : node->getChildNodes())
		{
			accumulateBoundingBoxes(child);
		}
	}
};

} // namespace
