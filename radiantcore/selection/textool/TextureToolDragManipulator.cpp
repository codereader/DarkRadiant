#include "TextureToolDragManipulator.h"

#include "itexturetoolmodel.h"
#include "math/Matrix3.h"
#include "pivot.h"
#include "igrid.h"
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
    const Vector2& devicePoint, unsigned int constraintFlags)
{
    auto device2Pivot = constructDevice2Pivot(pivot2world, view);
    auto current3D = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    Vector2 current(current3D.x(), current3D.y());

    auto diff = current - _start;

    if (constraintFlags & Constraint::Type1)
    {
        // Locate the index of the component carrying the largest abs value
        // Zero out the other component
        diff[fabs(diff.y()) > fabs(diff.x()) ? 0 : 1] = 0;
    }

    // Snap to grid if the constraint flag is set
    if (constraintFlags & Constraint::Grid)
    {
        auto gridSize = GlobalGrid().getGridSize(grid::Space::Texture);
        diff.x() = float_snapped(diff.x(), gridSize);
        diff.y() = float_snapped(diff.y(), gridSize);
    }

    _translateFunctor(diff);
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

    auto selectionMode = GlobalTextureToolSelectionSystem().getSelectionMode();

    GlobalTextureToolSceneGraph().foreachNode([&](const INode::Ptr& node)
    {
        if (selectionMode == SelectionMode::Surface)
        {
            node->testSelect(selectionPool, test);
        }
        else
        {
            auto componentSelectable = std::dynamic_pointer_cast<IComponentSelectable>(node);

            if (componentSelectable)
            {
                componentSelectable->testSelectComponents(selectionPool, test);
            }
        }

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

void TextureToolDragManipulator::renderComponents(const render::IRenderView& view, const Matrix4& pivot2World)
{}

void TextureToolDragManipulator::translateSelected(const Vector2& translation)
{
    auto transform = Matrix3::getTranslation(translation);

    if (GlobalTextureToolSelectionSystem().getSelectionMode() == SelectionMode::Surface)
    {
        GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
        {
            node->revertTransformation();
            node->transform(transform);
            return true;
        });
    }
    else
    {
        GlobalTextureToolSelectionSystem().foreachSelectedComponentNode([&](const INode::Ptr& node)
        {
            node->revertTransformation();

            auto componentTransformable = std::dynamic_pointer_cast<IComponentTransformable>(node);

            if (componentTransformable)
            {
                componentTransformable->transformComponents(transform);
            }
            return true;
        });
    }
}

}
