#pragma once

#include "ifiltermenu.h"
#include "gtkutil/preview/RenderPreview.h"
#include "inode.h"

#include "ui/menu/FiltersMenu.h"

namespace ui
{

// Forward decl.
class MapPreviewFilterObserver;
typedef boost::shared_ptr<MapPreviewFilterObserver> MapPreviewFilterObserverPtr;

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
private:
	// The filters menu
	IFilterMenuPtr _filtersMenu;

	// The filter observer
	MapPreviewFilterObserverPtr _filterObserver;

public:
	MapPreview();

	~MapPreview();

	// Get/set the map root to render
	void setRootNode(const scene::INodePtr& root);
	scene::INodePtr getRootNode();

	AABB getSceneBounds();

	// Gets called by a local helper object on each FilterSystem change
	void onFiltersChanged();

protected:
	bool onPreRender();

	RenderStateFlags getRenderFlagsFill();
};

} // namespace ui
