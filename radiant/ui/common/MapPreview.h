#pragma once

#include "wxutil/preview/RenderPreview.h"
#include "inode.h"

namespace ui
{

/**
 * greebo: This is a preview widget similar to the ui::ModelPreview class,
 * providing a GL render preview of a given root node.
 *
 * It comes with a Filters Menu included.
 *
 * Use the setRootNode() method to specify the subgraph to preview.
 */
class MapPreview :
	public wxutil::RenderPreview
{
public:
	MapPreview(wxWindow* parent);

	// Get/set the map root to render
    void setRootNode(const scene::IMapRootNodePtr& root);
    scene::IMapRootNodePtr getRootNode();

    AABB getSceneBounds() override;

protected:
    bool onPreRender() override;

    RenderStateFlags getRenderFlagsFill() override;
};

} // namespace ui
