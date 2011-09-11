#include "MapPreview.h"

#include "ifilter.h"
#include "iscenegraph.h"
#include "iuimanager.h"
#include "iscenegraphfactory.h"
#include "math/AABB.h"

#include <gtkmm/toolbar.h>

namespace ui
{

// Helper class, which notifies the MapPreview about a filter change
class MapPreviewFilterObserver :
	public FilterSystem::Observer
{
	MapPreview& _owner;
public:
	MapPreviewFilterObserver(MapPreview& owner) :
		_owner(owner)
	{}

	void onFiltersChanged() {
		_owner.onFiltersChanged();
	}
};
typedef boost::shared_ptr<MapPreviewFilterObserver> MapPreviewFilterObserverPtr;

MapPreview::MapPreview() :
	_filtersMenu(GlobalUIManager().createFilterMenu())
{
	Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
	toolbar->add(*_filtersMenu->getMenuBarWidget());

	addToolbar(*toolbar);

	// Add an observer to the FilterSystem to get notified about changes
	_filterObserver = MapPreviewFilterObserverPtr(new MapPreviewFilterObserver(*this));

	GlobalFilterSystem().addObserver(_filterObserver);
}

MapPreview::~MapPreview()
{
	GlobalFilterSystem().removeObserver(_filterObserver);
}

void MapPreview::setRootNode(const scene::INodePtr& root)
{
	getScene()->setRoot(root);

	if (getScene()->root() != NULL)
	{
		// Re-associate the rendersystem, we need to propagate this info to the nodes
		associateRenderSystem();

		// Trigger an initial update of the subgraph
		GlobalFilterSystem().updateSubgraph(getScene()->root());

		// Calculate camera distance so map is appropriately zoomed
		_camDist = -(getScene()->root()->worldAABB().getRadius() * 2.0f);

		_rotation = Matrix4::getIdentity();
	}
}

scene::INodePtr MapPreview::getRootNode()
{
	return getScene()->root();
}

AABB MapPreview::getSceneBounds()
{
	if (!getScene()->root()) return RenderPreview::getSceneBounds();

	return getScene()->root()->worldAABB();
}

void MapPreview::onFiltersChanged()
{
	// Sanity check
	if (getScene()->root() == NULL) return;

	GlobalFilterSystem().updateSubgraph(getScene()->root());
	_glWidget->queueDraw();
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
