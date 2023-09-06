#pragma once

#include "math/AABB.h"
#include "ilightnode.h"
#include "iorthoview.h"

/**
  SelectionPolicy for SelectByBounds
  Returns true if
*/
class SelectionPolicy_Complete_Tall
{
public:
	bool evaluate(const AABB& box, const scene::INodePtr& node) const
	{
		// Get the AABB of the visited instance
		AABB other = node->worldAABB();

		// greebo: Perform a special selection test for lights
		// as the small diamond should be tested against selection only
		ILightNodePtr light = Node_getLightNode(node);

		if (light )
		{
			other = light->getSelectAABB();
		}

		// Determine the viewtype
		OrthoOrientation viewType = GlobalOrthoViewManager().getActiveViewType();

		unsigned int axis1 = 0;
		unsigned int axis2 = 1;

		// Determine which axes have to be compared
		switch (viewType) {
			case OrthoOrientation::XY:
				axis1 = 0;
				axis2 = 1;
			break;
			case OrthoOrientation::YZ:
				axis1 = 1;
				axis2 = 2;
			break;
			case OrthoOrientation::XZ:
				axis1 = 0;
				axis2 = 2;
			break;
		};

		// Check if the AABB is contained
		auto dist1 = fabs(other.origin[axis1] - box.origin[axis1]) + fabs(other.extents[axis1]);
		auto dist2 = fabs(other.origin[axis2] - box.origin[axis2]) + fabs(other.extents[axis2]);

		return (dist1 < fabs(box.extents[axis1]) && dist2 < fabs(box.extents[axis2]));
	}
};

/**
  SelectionPolicy for SelectByBounds
  Returns true if box and the AABB of instance intersect
*/
class SelectionPolicy_Touching
{
public:
	bool evaluate(const AABB& box, const scene::INodePtr& node) const {
		const AABB& other(node->worldAABB());

		for (unsigned int i = 0; i < 3; ++i) {
            if (std::abs(box.origin[i] - other.origin[i]) > (box.extents[i] + other.extents[i])) {
				return false;
			}
		}

		return true;
	}
};

/**
  SelectionPolicy for SelectByBounds
  Returns true if the AABB of instance is inside box
*/
class SelectionPolicy_Inside
{
public:
	bool evaluate(const AABB& box, const scene::INodePtr& node) const
	{
		AABB other = node->worldAABB();

		// greebo: Perform a special selection test for lights
		// as the small diamond should be tested against selection only
		ILightNodePtr light = Node_getLightNode(node);

		if (light )
		{
			other = light->getSelectAABB();
		}

		for (unsigned int i = 0; i < 3; ++i) {
			if (std::abs(box.origin[i] - other.origin[i]) > (box.extents[i] - other.extents[i])) {
				return false;
			}
		}

		return true;
	}
};

/**
  SelectionPolicy for SelectByBounds
  Returns true if the AABB of instance is completely inside box.
  Bounding boxes with their planes flush on the selection box are discarded.
*/
class SelectionPolicy_FullyInside
{
public:
    bool evaluate(const AABB& box, const scene::INodePtr& node) const
    {
        AABB other = node->worldAABB();

        // greebo: Perform a special selection test for lights
        // as the small diamond should be tested against selection only
        ILightNodePtr light = Node_getLightNode(node);

        if (light)
        {
            other = light->getSelectAABB();
        }

        for (unsigned int i = 0; i < 3; ++i)
        {
            if (std::abs(box.origin[i] - other.origin[i]) >= (box.extents[i] - other.extents[i]))
            {
                return false;
            }
        }

        return true;
    }
};
