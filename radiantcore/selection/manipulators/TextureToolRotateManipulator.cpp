#include "TextureToolRotateManipulator.h"

#include "iselectiontest.h"
#include "itexturetoolmodel.h"
#include "selection/BestPoint.h"
#include "selection/SelectionPool.h"
#include "pivot.h"
#include "math/Matrix3.h"

namespace selection
{

void TextureRotator::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    auto device2Pivot = constructDevice2Pivot(pivot2world, view);
    auto pivotPoint = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    _start = Vector2(pivotPoint.x(), pivotPoint.y());

    auto length = _start.getLength();
    if (length > 0)
    {
        _start /= length;
    }
}

void TextureRotator::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraintFlags)
{
    auto device2Pivot = constructDevice2Pivot(pivot2world, view);

    auto current3D = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    auto current = Vector2(current3D.x(), current3D.y());
    
    auto length = current.getLength();
    if (length > 0)
    {
        current /= length;
    }

    _curAngle = acos(_start.dot(current));

    if (constraintFlags & Constraint::Type1)
    {
        _curAngle = float_snapped(_curAngle, 5 * c_DEG2RADMULT);
    }

    auto sign = _start.crossProduct(current) < 0 ? +1 : -1;
    _curAngle *= sign;

    _rotateFunctor(Vector2(pivot2world.tx(), pivot2world.ty()), _curAngle);
}

void TextureRotator::resetCurAngle()
{
    _curAngle = 0;
}

Vector3::ElementType TextureRotator::getCurAngle() const
{
    return _curAngle;
}

TextureToolRotateManipulator::TextureToolRotateManipulator() :
    _rotator(std::bind(&TextureToolRotateManipulator::rotateSelected, this, std::placeholders::_1, std::placeholders::_2)),
    _renderableCircle(8 << 3)
{
    draw_circle(8, 1.0f, &_renderableCircle.front(), RemapXYZ());
    _renderableCircle.setColour(Colour4b(255, 0, 0, 255));
}

std::size_t TextureToolRotateManipulator::getId() const
{
    return _id;
}

void TextureToolRotateManipulator::setId(std::size_t id)
{
    _id = id;
}

IManipulator::Type TextureToolRotateManipulator::getType() const
{
    return IManipulator::Rotate;
}

IManipulator::Component* TextureToolRotateManipulator::getActiveComponent()
{
    return &_rotator;
}

void TextureToolRotateManipulator::setSelected(bool select)
{
    _selectableZ.setSelected(select);
}

bool TextureToolRotateManipulator::isSelected() const
{
    return _selectableZ.isSelected();
}

void TextureToolRotateManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
{
    SelectionPool selector;

    test.BeginMesh(pivot2world, false);

    SelectionIntersection best;
    test.TestLineStrip(VertexPointer(&_renderableCircle.front().vertex, sizeof(VertexCb)), _renderableCircle.size(), best);

    if (best.isValid())
    {
        Selector_add(selector, _selectableZ);
    }

    if (!selector.empty())
    {
        selector.begin()->second->setSelected(true);
    }
}

void TextureToolRotateManipulator::renderComponents(const Matrix4& pivot2World)
{
    if (!_shader)
    {
        _shader = GlobalRenderSystem().capture("$WIRE_OVERLAY");
    }

    if (_renderableCircle.empty()) return;

    const auto& translation = pivot2World.tCol().getVector3();

    _renderableCircle.setColour(Colour4b(255, isSelected() ? 255 : 0, 0, 255));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslated(translation.x(), translation.y(), 0);

    // Enable point colours if required
    glEnableClientState(GL_COLOR_ARRAY);

    pointvertex_gl_array(&_renderableCircle.front());
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(_renderableCircle.size()));

    glDisableClientState(GL_COLOR_ARRAY);

    glPopMatrix();
}

void TextureToolRotateManipulator::rotateSelected(const Vector2& pivot, double angle)
{
    // Construct the full rotation around the pivot point
    auto transform = Matrix3::getTranslation(-pivot);
    transform.premultiplyBy(Matrix3::getRotation(angle));
    transform.premultiplyBy(Matrix3::getTranslation(pivot));

    GlobalTextureToolSceneGraph().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        node->applyTransformToSelected(transform);
        return true;
    });
}

}
