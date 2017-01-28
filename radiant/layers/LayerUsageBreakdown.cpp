#include "LayerUsageBreakdown.h"

#include "iscenegraph.h"
#include "ientity.h"
#include "iselection.h"
#include "scenelib.h"

namespace scene
{

namespace
{
	void addNodeMapping(LayerUsageBreakdown& bd, const scene::INodePtr& node)
	{
		scene::LayerList layers = node->getLayers();

		for (int layerId : layers)
		{
			assert(layerId >= 0); // we assume positive layer IDs here

			// Increase the counter of the corresponding layer slot by one
			bd[layerId]++;
		}
	}
}

LayerUsageBreakdown LayerUsageBreakdown::CreateFromScene(bool includeHidden)
{
	LayerUsageBreakdown bd;

	InitialiseVector(bd);

	GlobalSceneGraph().foreachNode([&](const scene::INodePtr& node)
	{
		// Filter out any hidden nodes, unless we want them
		if (!includeHidden && !node->visible()) return false;

		// Consider only entities and primitives
		if (!Node_isPrimitive(node) && !Node_isEntity(node)) return true;

		addNodeMapping(bd, node);

		return true;
	});

	return bd;
}

LayerUsageBreakdown LayerUsageBreakdown::CreateFromSelection()
{
	LayerUsageBreakdown bd;

	InitialiseVector(bd);

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		addNodeMapping(bd, node);
	});

	return bd;
}

void LayerUsageBreakdown::InitialiseVector(LayerUsageBreakdown& bd)
{
	// Start with a reasonably large memory block
	bd.reserve(64);

	// We resize the vector to 0 length, such that each resize() call
	// below will fill the vector with zeros
	bd.resize(0, 0);

	GlobalLayerSystem().foreachLayer([&](int layerId, const std::string& layerName)
	{
		if (layerId >= static_cast<int>(bd.size()))
		{
			bd.resize(layerId+1, 0);
		}
	});
}

}
