#pragma once

#include "itexturetoolmodel.h"
#include "math/AABB.h"
#include "math/Matrix3.h"

namespace selection
{

namespace algorithm
{

class TextureBoundsAccumulator
{
private:
    AABB _bounds;

public:
    // Textool node visitor
    bool operator()(const textool::INode::Ptr& node);

    const AABB& getBounds() const
    {
        return _bounds;
    }
};

// Flips all the visited node about the given axis and the given flip center point (in UV space)
class TextureFlipper
{
private:
    Matrix3 _transform;

public:
    TextureFlipper(const Vector2& flipCenter, int axis);

    // Function operator, makes this class suitable to pass to e.g. ITextureToolSelectionSystem.foreachSelected()
    bool operator()(const textool::INode::Ptr& node);
};

}

}
