#include "Texturing.h"

#include "selection/textool/FaceNode.h"
#include "selection/textool/PatchNode.h"
#include "messages/TextureChanged.h"

#include "ipatch.h"
#include "ibrush.h"

namespace selection
{

namespace algorithm
{

namespace
{

inline void applyTransform(const textool::INode::Ptr& node, const Matrix3& transform)
{
    node->beginTransformation();
    node->transform(transform);
    node->commitTransformation();
}

}

TextureNodeProcessor::operator std::function<bool(const textool::INode::Ptr& node)>()
{
    return [this](const textool::INode::Ptr& node)
    {
        return processNode(node);
    };
}

bool TextureBoundsAccumulator::processNode(const textool::INode::Ptr& node)
{
    _bounds.includeAABB(node->localAABB());
    return true;
}

bool TextureNodeManipulator::processNode(const textool::INode::Ptr& node)
{
    applyTransform(node, _transform);
    _numProcessedNodes++;
    return true;
}

TextureNodeManipulator::TextureNodeManipulator() :
    _numProcessedNodes(0)
{}

TextureNodeManipulator::~TextureNodeManipulator()
{
    // Dispatch the texture changed signal if we processed at least one node
    if (_numProcessedNodes)
    {
        radiant::TextureChangedMessage::Send();
    }
}

TextureFlipper::TextureFlipper(const Vector2& flipCenter, int axis)
{
    auto flipMatrix = Matrix3::getIdentity();

    if (axis == 0)
    {
        flipMatrix.xx() = -1;
    }
    else // axis == 1
    {
        flipMatrix.yy() = -1;
    }

    _transform = Matrix3::getTranslation(-flipCenter);
    _transform.premultiplyBy(flipMatrix);
    _transform.premultiplyBy(Matrix3::getTranslation(+flipCenter));
}

void TextureFlipper::FlipNode(const textool::INode::Ptr& node, int flipAxis)
{
    const auto& bounds = node->localAABB();
    TextureFlipper flipper({ bounds.origin.x(), bounds.origin.y() }, flipAxis);

    flipper.processNode(node);
}

void TextureFlipper::FlipPatch(IPatch& patch, int flipAxis)
{
    FlipNode(std::make_shared<textool::PatchNode>(patch), flipAxis);
}

void TextureFlipper::FlipFace(IFace& face, int flipAxis)
{
    FlipNode(std::make_shared<textool::FaceNode>(face), flipAxis);
}

// Rotation

TextureRotator::TextureRotator(const Vector2& pivot, double angle, double textureAspectRatio)
{
    _transform = Matrix3::getTranslation(-pivot);
    _transform.premultiplyBy(Matrix3::getScale({ textureAspectRatio, 1 }));
    _transform.premultiplyBy(Matrix3::getRotation(angle));
    _transform.premultiplyBy(Matrix3::getScale({ 1 / textureAspectRatio, 1 }));
    _transform.premultiplyBy(Matrix3::getTranslation(pivot));
}

void TextureRotator::RotatePatch(IPatch& patch, double angle)
{
    RotateNode(std::make_shared<textool::PatchNode>(patch), angle, patch.getTextureAspectRatio());
}

void TextureRotator::RotateFace(IFace& face, double angle)
{
    RotateNode(std::make_shared<textool::FaceNode>(face), angle, face.getTextureAspectRatio());
}

void TextureRotator::RotateNode(const textool::INode::Ptr& node, double angle, double textureAspectRatio)
{
    const auto& bounds = node->localAABB();
    TextureRotator rotator({ bounds.origin.x(), bounds.origin.y() }, angle, textureAspectRatio);

    rotator.processNode(node);
}

// Scaling

TextureScaler::TextureScaler(const Vector2& pivot, const Vector2& scale)
{
    _transform = Matrix3::getTranslation(-pivot);
    _transform.premultiplyBy(Matrix3::getScale(scale));
    _transform.premultiplyBy(Matrix3::getTranslation(pivot));
}

void TextureScaler::ScalePatch(IPatch& patch, const Vector2& scale)
{
    ScaleNode(std::make_shared<textool::PatchNode>(patch), scale);
}

void TextureScaler::ScaleFace(IFace& face, const Vector2& scale)
{
    ScaleNode(std::make_shared<textool::FaceNode>(face), scale);
}

void TextureScaler::ScaleNode(const textool::INode::Ptr& node, const Vector2& scale)
{
    const auto& bounds = node->localAABB();
    TextureScaler scaler({ bounds.origin.x(), bounds.origin.y() }, scale);

    scaler.processNode(node);
}

// Normalise

TextureNormaliser::TextureNormaliser(const Vector2& boundsCenter)
{
    Vector2 roundedCenter
    {
        boundsCenter.x() > 0 ? floor(boundsCenter.x()) : ceil(boundsCenter.x()),
        boundsCenter.y() > 0 ? floor(boundsCenter.y()) : ceil(boundsCenter.y())
    };

    _transform = Matrix3::getTranslation(-roundedCenter);
}

void TextureNormaliser::NormalisePatch(IPatch& patch)
{
    NormaliseNode(std::make_shared<textool::PatchNode>(patch));
}

void TextureNormaliser::NormaliseFace(IFace& face)
{
    NormaliseNode(std::make_shared<textool::FaceNode>(face));
}

void TextureNormaliser::NormaliseNode(const textool::INode::Ptr& node)
{
    const auto& bounds = node->localAABB();
    TextureNormaliser normaliser({ bounds.origin.x(), bounds.origin.y() });

    normaliser.processNode(node);
}

}

}
