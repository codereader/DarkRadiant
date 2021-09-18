#include "TextureToolDragManipulator.h"

#include "itexturetoolmodel.h"
#include "math/Matrix3.h"
#include "pivot.h"
#include "selection/SelectionPool.h"

namespace textool
{

void TextureTranslator::beginTransformation(const Matrix4& pivot2world, 
    const VolumeTest& view, const Vector2& devicePoint)
{
    auto device2Pivot = constructDevice2Pivot(pivot2world, view);
    auto pivotPoint = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    _start = Vector2(pivotPoint.x(), pivotPoint.y());
}

void TextureTranslator::transform(const Matrix4& pivot2world, const VolumeTest& view,
    const Vector2& devicePoint, unsigned int constraints)
{
    auto device2Pivot = constructDevice2Pivot(pivot2world, view);
    auto current3D = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    Vector2 current(current3D.x(), current3D.y());

    _translateFunctor(current - _start);
}

TextureToolDragManipulator::TextureToolDragManipulator() :
    _translator(std::bind(&TextureToolDragManipulator::translateSelected, this, std::placeholders::_1))
{}

std::size_t TextureToolDragManipulator::getId() const
{
    return _id;
}

void TextureToolDragManipulator::setId(std::size_t id)
{
    _id = id;
}

selection::IManipulator::Type TextureToolDragManipulator::getType() const
{
    return IManipulator::Drag;
}

selection::IManipulator::Component* TextureToolDragManipulator::getActiveComponent()
{
    return &_translator;
}

void TextureToolDragManipulator::setSelected(bool select)
{
    _selectable.setSelected(select);
}

bool TextureToolDragManipulator::isSelected() const
{
    return _selectable.isSelected();
}

void TextureToolDragManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
{
    selection::SelectionPool selectionPool;

    GlobalTextureToolSceneGraph().foreachNode([&](const INode::Ptr& node)
    {
        node->testSelect(selectionPool, test);
        return true;
    });

    _selectable.setSelected(false);

    // The drag manipulator returns positive if we our test hit a selected node
    for (const auto& pair : selectionPool)
    {
        if (pair.second->isSelected())
        {
            _selectable.setSelected(true);
            break;
        }
    }
}

void TextureToolDragManipulator::renderComponents(const Matrix4& pivot2World)
{}

void TextureToolDragManipulator::translateSelected(const Vector2& translation)
{
    auto transform = Matrix3::getTranslation(translation);

    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        node->revertTransformation();
        node->applyTransformToSelected(transform);
        return true;
    });
}

}
