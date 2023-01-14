#pragma once

#include <unordered_set>

#include "ilayer.h"
#include "entitylib.h"

namespace scene
{

/**
 * Node visitor implementation, setting any node's selection status
 * to the given value, as long as any of the node's layers is occurring in the given set.
 */
class SetLayerSelectedWalker :
	public NodeVisitor
{
private:
    const std::unordered_set<int>& _layerIds;
	bool _selected;

public:
	SetLayerSelectedWalker(const std::unordered_set<int>& layerIds, bool selected) :
        _layerIds(layerIds),
		_selected(selected)
	{}

	bool pre(const INodePtr& node) override
	{
        // Skip hidden nodes, but only when we're selecting
        // When de-selecting we don't care whether the target node can be seen
        if (_selected && !node->visible()) return false;

        // Skip the worldspawn
        if (Node_isWorldspawn(node)) return true;

        const auto& layers = node->getLayers();

        for (auto layerId : layers)
        {
            if (_layerIds.count(layerId) != 0)
            {
                Node_setSelected(node, _selected);
                break;
            }
        }

        return true;
	}
};

} // namespace scene
