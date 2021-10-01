#pragma once

#include "itexturetoolmodel.h"
#include "math/AABB.h"
#include "math/Matrix3.h"

namespace selection
{

namespace algorithm
{

class TextureNodeManipulator
{
protected:
    TextureNodeManipulator()
    {}

public:
    // Conversion operator, to be able to pass an instance reference directly to 
    // ITextureToolSelectionSystem.foreachSelected() without having to set up the std::bind
    operator std::function<bool(const textool::INode::Ptr& node)>();

    // Required processor method to be implemented by subclasses
    virtual bool processNode(const textool::INode::Ptr& node) = 0;
};

class TextureBoundsAccumulator :
    public TextureNodeManipulator
{
private:
    AABB _bounds;

public:
    bool processNode(const textool::INode::Ptr& node) override;

    const AABB& getBounds() const
    {
        return _bounds;
    }
};

// Flips all the visited node about the given axis and the given flip center point (in UV space)
class TextureFlipper :
    public TextureNodeManipulator
{
private:
    Matrix3 _transform;

public:
    TextureFlipper(const Vector2& flipCenter, int axis);

    bool processNode(const textool::INode::Ptr& node) override;

    // Directly flip the texture of the given patch
    static void FlipPatch(IPatch& patch, int flipAxis);
};

}

}
