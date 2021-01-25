#include "MapPreview.h"

#include "ifilter.h"
#include "iscenegraph.h"
#include "iuimanager.h"
#include "iscenegraphfactory.h"
#include "math/AABB.h"
#include "scene/PrefabBoundsAccumulator.h"

namespace ui
{

MapPreview::MapPreview(wxWindow* parent) : 
	RenderPreview(parent)
{}

void MapPreview::setRootNode(const scene::IMapRootNodePtr& root)
{
	getScene()->setRoot(root);

	if (getScene()->root() != NULL)
	{
		// Re-associate the rendersystem, we need to propagate this info to the nodes
		associateRenderSystem();

		// Trigger an initial update of the subgraph
		GlobalFilterSystem().updateSubgraph(getScene()->root());

        // Reset the model rotation
        resetModelRotation();

		// Calculate camera distance so map is appropriately zoomed
        auto sceneBounds = getSceneBounds();
        auto distance = sceneBounds.getRadius() * 3.0f;

        setViewOrigin(sceneBounds.getOrigin() + Vector3(-0.5, -0.8, 0.4) * distance);
        setViewAngles(Vector3(26, 300, 0));
	}
}

scene::IMapRootNodePtr MapPreview::getRootNode()
{
	return getScene()->root();
}

AABB MapPreview::getSceneBounds()
{
	if (!getScene()->root()) return RenderPreview::getSceneBounds();

    scene::PrefabBoundsAccumulator accumulator;
    getScene()->root()->traverseChildren(accumulator);

	return accumulator.getBounds();
}

bool MapPreview::onPreRender()
{
	// Trigger scenegraph instantiation
	getScene();

	return getScene()->root() != NULL;
}

RenderStateFlags MapPreview::getRenderFlagsFill()
{
	return RenderPreview::getRenderFlagsFill() | RENDER_DEPTHWRITE | RENDER_DEPTHTEST;
}

} // namespace ui
