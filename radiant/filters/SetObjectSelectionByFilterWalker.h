#pragma once

#include "inode.h"
#include "ifilter.h"
#include "ipatch.h"
#include "ibrush.h"
#include "ientity.h"
#include "iselectable.h"

#include "XMLFilter.h"

namespace filters
{

class SetObjectSelectionByFilterWalker :
	public scene::NodeVisitor
{
private:
	XMLFilter& _filter;
	bool _selectIfFiltered;

public:
	SetObjectSelectionByFilterWalker(XMLFilter& filter, bool selectIfFiltered) :
		_filter(filter),
		_selectIfFiltered(selectIfFiltered)
	{}

	bool pre(const scene::INodePtr& node) override
	{
		if (!node->visible())
		{
			return true;
		}

		// Check entity eclass and spawnargs
		if (Node_isEntity(node))
		{
			Entity* entity = Node_getEntity(node);

			bool isVisible = _filter.isEntityVisible(FilterRule::TYPE_ENTITYCLASS, *entity) &&
				_filter.isEntityVisible(FilterRule::TYPE_ENTITYKEYVALUE, *entity);

			if (!isVisible)
			{
				// The filter would hide this item, apply the action
				Node_setSelected(node, _selectIfFiltered);
			}

			// If the entity is affected, don't traverse its child nodes
			return isVisible;
		}
		
		// greebo: Check visibility of Patches
		if (Node_isPatch(node))
		{
			// Check by object type "patch" and by the patch's material
			bool isVisible = _filter.isVisible(FilterRule::TYPE_OBJECT, "patch") &&
				materialIsVisible(Node_getIPatch(node)->getShader());

			if (!isVisible)
			{
				// The filter would hide this item, apply the action
				Node_setSelected(node, _selectIfFiltered);
			}
		}
		// greebo: Check visibility of Brushes
		else if (Node_isBrush(node))
		{
			// Check by object type "brush" and by the brush materials
			bool isVisible = _filter.isVisible(FilterRule::TYPE_OBJECT, "brush") &&
				allBrushMaterialsVisible(Node_getIBrush(node));

			if (!isVisible)
			{
				// The filter would hide this item (at least partially), apply the action
				Node_setSelected(node, _selectIfFiltered);
			}
		}

		// Continue the traversal
		return true;
	}

private:
	bool materialIsVisible(const std::string& materialName)
	{
		return _filter.isVisible(FilterRule::TYPE_TEXTURE, materialName);
	}

	bool allBrushMaterialsVisible(IBrush* brush)
	{
		for (std::size_t i = 0; i < brush->getNumFaces(); ++i)
		{
			if (!materialIsVisible(brush->getFace(i).getShader()))
			{
				return false;
			}
		}

		return brush->hasContributingFaces();
	}
};

}
