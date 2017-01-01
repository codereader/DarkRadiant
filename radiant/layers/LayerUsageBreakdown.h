#pragma once

#include <vector>

namespace scene
{

/**
 * This object contains the information about how many
 * nodes are present in each layer.
 * It's implemented by means of a vector, indexable by layer ID.
 *
 * Use the static methods to populate this class from the
 * selection or from all scene objects.
 *
 * Retrieve the object count for a given layer like this:
 * breakdown[<layerID>] = <count>
 */
class LayerUsageBreakdown :
	public std::vector<std::size_t>
{
private:
	LayerUsageBreakdown()
	{}

public:
	// Creates the layer summary based on the current selection
	static LayerUsageBreakdown CreateFromSelection();

	// Creates the layer summary based on the nodes in the GlobalScenegraph
	// NOTE: only primitives and entities are considered.
	// If includeHidden is set to true, currently invisible items are added
	static LayerUsageBreakdown CreateFromScene(bool includeHidden);

private:
	// Makes sure the vector is large enough to host all layer IDs
	// Sets all counts back to 0
	static void InitialiseVector(LayerUsageBreakdown& bd);
};

}
