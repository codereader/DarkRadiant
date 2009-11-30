#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "irender.h"
#include "irenderable.h"
#include "render/frontend/RenderHighlighted.h"

/**
 * Test the visibility of the given Node by intersecting with the given
 * VolumeTest, subject to the visibility of the parent. If the parent's
 * intersection is partial, this node is tested, otherwise the parent's
 * visibility is used instead.
 */
/*inline 
VolumeIntersectionValue Cullable_testVisible(const scene::INodePtr& node, 
											 const VolumeTest& volume, 
											 VolumeIntersectionValue parent)
{
	// Check for partial intersection with the parent
	if(parent == VOLUME_PARTIAL) 
    {
		// Parent has partial visibility, so test this Instance and return
		// its result
		CullablePtr cullable = boost::dynamic_pointer_cast<Cullable>(node);
		if (cullable != NULL) {
			return cullable->intersectVolume(volume, node->localToWorld());
		}
	}
	
	// Parent was not partially visible or this instance is not Cullable, so
	// pass back the parent's visibility
	return parent;
}*/

/**
 * Scene render function. Uses the visibility walkers to traverse the scene
 * graph and submit all visible objects to the provided RenderableCollector.
 */
inline void Scene_Render(RenderableCollector& collector, const VolumeTest& volume) {

	// Instantiate a new walker class
	RenderHighlighted renderHighlightWalker(collector, volume);

	// Submit renderables from scene graph
	GlobalSceneGraph().foreachNodeInVolume(volume, renderHighlightWalker);
	
	// Submit renderables directly attached to the ShaderCache
	GlobalRenderSystem().forEachRenderable(
		RenderHighlighted::RenderCaller(RenderHighlighted(collector, volume)));
}

#endif /* _RENDERER_H_ */
