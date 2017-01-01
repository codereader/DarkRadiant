#include "LayerUsageBreakdown.h"

namespace scene
{

LayerUsageBreakdown LayerUsageBreakdown::CreateFromSelection()
{
	LayerUsageBreakdown bd;

	InitialiseVector(bd);

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		scene::LayerList layers = node->getLayers();

		for (int layerId : layers)
		{
			assert(layerId >= 0); // we assume positive layer IDs here

			// Increase the counter of the corresponding layer slot by one
			bd[layerId]++;
		}
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
		if (layerId >= bd.size())
		{
			bd.resize(layerId+1, 0);
		}
	});
}

}
