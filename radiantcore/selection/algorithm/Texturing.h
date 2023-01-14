#pragma once

#include "itexturetoolmodel.h"
#include "math/AABB.h"
#include "math/Matrix3.h"

class IPatch;
class IFace;

namespace selection
{

namespace algorithm
{

class TextureNodeProcessor
{
protected:
    TextureNodeProcessor() = default;

public:
    // Conversion operator, to be able to pass an instance reference directly to 
    // ITextureToolSelectionSystem.foreachSelected() without having to set up the std::bind
    operator std::function<bool(const textool::INode::Ptr& node)>();

    // Required processor method to be implemented by subclasses
    virtual bool processNode(const textool::INode::Ptr& node) = 0;
};

class TextureBoundsAccumulator :
    public TextureNodeProcessor
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

// Will dispatch a TextureChangedMessage after processing at least one node
class TextureNodeManipulator :
    public TextureNodeProcessor
{
protected:
    Matrix3 _transform;
    std::size_t _numProcessedNodes;

    TextureNodeManipulator();

public:
    virtual ~TextureNodeManipulator();

    const Matrix3& getTransform() const;

    bool processNode(const textool::INode::Ptr& node) override;
};

// Flips all the visited node about the given axis and the given flip center point (in UV space)
class TextureFlipper :
    public TextureNodeManipulator
{
public:
    TextureFlipper(const Vector2& flipCenter, int axis);

    // Directly flip the texture of the given patch
    static void FlipPatch(IPatch& patch, int flipAxis);

    // Directly flip the texture of the given face
    static void FlipFace(IFace& face, int flipAxis);

private:
    static void FlipNode(const textool::INode::Ptr& node, int flipAxis);
};

class TextureRotator :
    public TextureNodeManipulator
{
public:
    TextureRotator(const Vector2& pivot, double angle, double aspect);

    // Directly rotate the texture of the given patch around its UV center
    static void RotatePatch(IPatch& patch, double angle);

    // Directly rotate the texture of the given face around its UV center
    static void RotateFace(IFace& face, double angle);

private:
    static void RotateNode(const textool::INode::Ptr& node, double angle, double aspect);
};

class TextureScaler :
    public TextureNodeManipulator
{
public:
    // A scale component value of 1.0 == 100%
    TextureScaler(const Vector2& pivot, const Vector2& scale);

    // Directly scale the texture of the given patch with its UV center as pivot
    static void ScalePatch(IPatch& patch, const Vector2& scale);

    // Directly scale the texture of the given face with its UV center as pivot
    static void ScaleFace(IFace& face, const Vector2& scale);

private:
    static void ScaleNode(const textool::INode::Ptr& node, const Vector2& scale);
};

// Will shift the texture coordinates towards the origin,
// using the integer part of the given bounds center
class TextureNormaliser :
    public TextureNodeManipulator
{
public:
    TextureNormaliser(const Vector2& boundsCenter);

    // Normalise the texture of the given patch
    static void NormalisePatch(IPatch& patch);

    // Normalise the texture of the given face
    static void NormaliseFace(IFace& face);

private:
    static void NormaliseNode(const textool::INode::Ptr& node);
};

}

}
