#pragma once

#include "gtkutil/preview/RenderPreview.h"
#include "inode.h"

namespace ui
{

/**
 * greebo: This is a preview widget similar to the ui::ModelPreview class,
 * providing a GL render preview of a given root node.
 *
 * It comes with a Filters Menu included. Use the GtkWidget* operator
 * to retrieve the widget for packing into a parent container.
 *
 * Use the setRootNode() method to specify the subgraph to preview.
 */
class MapPreview :
	public gtkutil::RenderPreview
{
public:
	MapPreview();

	// Get/set the map root to render
	void setRootNode(const scene::INodePtr& root);
	scene::INodePtr getRootNode();

	AABB getSceneBounds();

protected:
	bool onPreRender();

	RenderStateFlags getRenderFlagsFill();
};

} // namespace ui
