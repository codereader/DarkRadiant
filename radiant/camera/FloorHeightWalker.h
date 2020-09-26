#pragma once

#include "inode.h"
#include "gamelib.h"

namespace camera
{

class FloorHeightWalker :
    public scene::NodeVisitor
{
private:
    float _current;
    float& _bestUp;
    float& _bestDown;

public:
    FloorHeightWalker(float current, float& bestUp, float& bestDown) :
        _current(current),
        _bestUp(bestUp),
        _bestDown(bestDown)
    {
        _bestUp = game::current::getValue<float>("/defaults/maxWorldCoord");
        _bestDown = -game::current::getValue<float>("/defaults/maxWorldCoord");
    }

    bool pre(const scene::INodePtr& node)
    {
        if (!node->visible()) return false; // don't traverse hidden nodes

        if (Node_isBrush(node)) // this node is a floor
        {
            const AABB& aabb = node->worldAABB();

            float floorHeight = aabb.origin.z() + aabb.extents.z();

            if (floorHeight > _current&& floorHeight < _bestUp)
            {
                _bestUp = floorHeight;
            }

            if (floorHeight < _current && floorHeight > _bestDown)
            {
                _bestDown = floorHeight;
            }

            return false;
        }

        return true;
    }
};

}
