#include "TextureToolDragManipulator.h"

#include "itexturetoolmodel.h"
#include "math/Matrix3.h"
#include "pivot.h"
#include "igrid.h"
#include "selection/SelectionPool.h"
#include "selection/algorithm/Texturing.h"

namespace textool
{

namespace detail
{

// Constrains the vector as defined by the given set of flags
Vector2 getConstrainedDelta(const Vector2& vector, unsigned int constraintFlags)
{
    auto diff = vector;

    // Axis constraints
    if (constraintFlags & selection::IManipulator::Component::Constraint::Type1)
    {
        // Locate the index of the component carrying the largest abs value
        // Zero out the other component
        diff[fabs(diff.y()) > fabs(diff.x()) ? 0 : 1] = 0;
    }

    // Grid constraint
    if (constraintFlags & selection::IManipulator::Component::Constraint::Grid)
    {
        auto gridSize = GlobalGrid().getGridSize(grid::Space::Texture);
        diff.x() = float_snapped(diff.x(), gridSize);
        diff.y() = float_snapped(diff.y(), gridSize);
    }

    return diff;
}

}

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

    auto diff = detail::getConstrainedDelta(current - _start, constraintFlags);

    _translateFunctor(diff);
}

void TextureDragResizer::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    auto device2World = constructDevice2Pivot(pivot2world, view).getPremultipliedBy(pivot2world);

    // Remember the starting poing of the operation
    auto start = device2World.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    _start = Vector2(start.x(), start.y());

    // Get the selection bounds to figure out the "opposite" point we'll be using as pivot
    selection::algorithm::TextureBoundsAccumulator accumulator;
    GlobalTextureToolSelectionSystem().foreachSelectedNode(accumulator);

    // Move the bounds to pivot space
    const auto& bounds = accumulator.getBounds();

    _scalePivot = FindFarthestCorner(bounds, _start);

    auto boundsMin = bounds.getOrigin() - bounds.getExtents();
    auto boundsMax = bounds.getOrigin() + bounds.getExtents();

    _scaleMask = Vector2(
        _start.x() > boundsMax.x() || _start.x() < boundsMin.x() ? 1 : 0,
        _start.y() > boundsMax.y() || _start.y() < boundsMin.y() ? 1 : 0
    );

    _startingBoundsExtents = Vector2(bounds.getExtents().x(), bounds.getExtents().y());
}

Vector2 TextureDragResizer::FindFarthestCorner(const AABB& bounds, const Vector2& start)
{
    Vector3 points[8];
    bounds.getCorners(points);

    Vector2 result;
    Vector2::ElementType greatestDistance = -1;

    for (auto i = 0; i < 8; ++i)
    {
        auto candidatePoint = Vector2(points[i].x(), points[i].y());
        auto candidateDistance = (candidatePoint - start).getLengthSquared();

        if (candidateDistance > greatestDistance)
        {
            greatestDistance = candidateDistance;
            result = candidatePoint;
        }
    }

    return result;
}

void TextureDragResizer::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraintFlags)
{
    auto device2World = constructDevice2Pivot(pivot2world, view).getPremultipliedBy(pivot2world);

    // Calculate the current point in UV space
    auto currentWorldPoint = device2World.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    auto current = Vector2(currentWorldPoint.x(), currentWorldPoint.y());

    auto diff = detail::getConstrainedDelta(current - _start, constraintFlags);

    // Consider the side of the pivot we're moving the mouse
    auto factor = Vector2(
        _scalePivot.x() > _start.x() ? -1.0 : 1.0,
        _scalePivot.y() > _start.y() ? -1.0 : 1.0
    );

    // Calculate how much the selection bounds should grow (in UV coordinates)
    // The extents are only covering half of the bounds, so cut the distance in half to compensate
    auto scale = (_startingBoundsExtents + factor * diff * 0.5) / _startingBoundsExtents;

    // Set those components we don't scale to 1.0
    Vector2 constrainedScale(
        _scaleMask.x() > 0 ? fabs(scale.x()) : 1.0,
        _scaleMask.y() > 0 ? fabs(scale.y()) : 1.0
    );

    _scaleFunctor(constrainedScale, _scalePivot);
}

TextureToolDragManipulator::TextureToolDragManipulator() :
    _translator(std::bind(&TextureToolDragManipulator::translateSelected, this, std::placeholders::_1)),
    _resizer(std::bind(&TextureToolDragManipulator::scaleSelected, this, std::placeholders::_1, std::placeholders::_2))
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
    return _translateSelectable.isSelected() ? &_translator : static_cast<Component*>(& _resizer);
}

void TextureToolDragManipulator::setSelected(bool select)
{
    _translateSelectable.setSelected(select);
    _scaleSelectable.setSelected(select);
}

bool TextureToolDragManipulator::isSelected() const
{
    return _translateSelectable.isSelected() || _scaleSelectable.isSelected();
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

    _translateSelectable.setSelected(false);

    // The drag manipulator returns positive if we our test hit a selected node
    for (const auto& pair : selectionPool)
    {
        if (pair.second->isSelected())
        {
            _translateSelectable.setSelected(true);
            return; // done here
        }
    }

    // No selectable has been directly hit, check if the mouse is on any side of the selection AABB
    if (GlobalTextureToolSelectionSystem().getSelectionMode() == SelectionMode::Surface)
    {
        testSelectDragResize(test, pivot2world);
    }
}

void TextureToolDragManipulator::renderComponents(const render::IRenderView& view, const Matrix4& pivot2World)
{}

void TextureToolDragManipulator::translateSelected(const Vector2& translation)
{
    auto transform = Matrix3::getTranslation(translation);

    if (GlobalTextureToolSelectionSystem().getSelectionMode() == SelectionMode::Surface)
    {
        GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const INode::Ptr& node)
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

void TextureToolDragManipulator::scaleSelected(const Vector2& scale, const Vector2& pivot)
{
    if (GlobalTextureToolSelectionSystem().getSelectionMode() != SelectionMode::Surface)
    {
        return;
    }

    selection::algorithm::TextureScaler scaler(pivot, scale);

    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const INode::Ptr& node)
    {
        node->revertTransformation();
        node->transform(scaler.getTransform());
        return true;
    });
}

void TextureToolDragManipulator::testSelectDragResize(SelectionTest& test, const Matrix4& pivot2world)
{
    // Get the selection bounds to get the sides
    selection::algorithm::TextureBoundsAccumulator accumulator;
    GlobalTextureToolSelectionSystem().foreachSelectedNode(accumulator);

    const auto& bounds = accumulator.getBounds();

    // If nothing is selected, the bounds will remain invalid
    if (!bounds.isValid()) return;

    // Get the point in texture space the test is pointing at
    auto hitPoint = pivot2world.transformPoint(test.getNear());

    // If the hit point is outside the bounds, the selection test has passed
    auto pointIsOutsideBounds = !bounds.contains(AABB::createFromMinMax(hitPoint, hitPoint));
    _scaleSelectable.setSelected(pointIsOutsideBounds);
}

}
