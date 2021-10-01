#include "Texturing.h"

#include "selection/textool/FaceNode.h"
#include "selection/textool/PatchNode.h"

namespace selection
{

namespace algorithm
{

TextureNodeManipulator::operator std::function<bool(const textool::INode::Ptr& node)>()
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

bool TextureFlipper::processNode(const textool::INode::Ptr& node)
{
    node->beginTransformation();
    node->transform(_transform);
    node->commitTransformation();
    return true;
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

}

}
