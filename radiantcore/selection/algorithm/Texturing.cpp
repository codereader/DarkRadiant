#include "Texturing.h"

namespace selection
{

namespace algorithm
{

bool TextureBoundsAccumulator::operator()(const textool::INode::Ptr& node)
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

bool TextureFlipper::operator()(const textool::INode::Ptr& node)
{
    node->beginTransformation();
    node->transform(_transform);
    node->commitTransformation();
    return true;
}

}

}
