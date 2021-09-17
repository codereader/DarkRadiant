#include "TextureToolRotateManipulator.h"

#include "iselectiontest.h"
#include "itexturetoolmodel.h"
#include "selection/BestPoint.h"
#include "selection/SelectionPool.h"
#include "pivot.h"
#include "math/Matrix3.h"

namespace textool
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
    _current = Vector2(current3D.x(), current3D.y());
    
    auto length = _current.getLength();
    if (length > 0)
    {
        _current /= length;
    }

    _curAngle = acos(_start.dot(_current));

    if (constraintFlags & Constraint::Type1)
    {
        _curAngle = float_snapped(_curAngle, 5 * c_DEG2RADMULT);
    }

    auto sign = _start.crossProduct(_current) < 0 ? +1 : -1;
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

const Vector2& TextureRotator::getStartDirection() const
{
    return _start;
}

const Vector2& TextureRotator::getCurrentDirection() const
{
    return _current;
}

TextureToolRotateManipulator::TextureToolRotateManipulator(textool::TextureToolManipulationPivot& pivot) :
    _pivot(pivot),
    _rotator(std::bind(&TextureToolRotateManipulator::rotateSelected, this, std::placeholders::_1, std::placeholders::_2)),
    _renderableCircle(8 << 3),
    _circleRadius(1.0f)
{
    draw_circle(8, _circleRadius, &_renderableCircle.front(), RemapXYZ());
    _renderableCircle.setColour(Colour4b(200, 200, 200, 200));
}

std::size_t TextureToolRotateManipulator::getId() const
{
    return _id;
}

void TextureToolRotateManipulator::setId(std::size_t id)
{
    _id = id;
}

selection::IManipulator::Type TextureToolRotateManipulator::getType() const
{
    return IManipulator::Rotate;
}

selection::IManipulator::Component* TextureToolRotateManipulator::getActiveComponent()
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
    selection::SelectionPool selector;

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

        auto manipulatorFontStyle = registry::getValue<std::string>(selection::RKEY_MANIPULATOR_FONTSTYLE) == "Sans" ?
            IGLFont::Style::Sans : IGLFont::Style::Mono;
        auto manipulatorFontSize = registry::getValue<int>(selection::RKEY_MANIPULATOR_FONTSIZE);

        _glFont = GlobalOpenGL().getFont(manipulatorFontStyle, manipulatorFontSize);
    }

    if (_renderableCircle.empty()) return;

    const auto& translation = pivot2World.tCol().getVector3();

    _renderableCircle.setColour(isSelected() ? 
        Colour4b(255, 255, 0, 200) : Colour4b(200, 200, 200, 200));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslated(translation.x(), translation.y(), 0);

    auto angle = _rotator.getCurAngle();

    if (_selectableZ.isSelected())
    {
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.3f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        glColor3f(0.6f, 0.6f, 0.6f);

        glBegin(GL_TRIANGLE_FAN);

        glVertex3d(0, 0, 0);

        auto startingPointOnCircle = _rotator.getStartDirection() * _circleRadius;
        glVertex3d(startingPointOnCircle.x(), startingPointOnCircle.y(), 0);

        // 3 degree steps
        auto stepSize = degrees_to_radians(3.0);

        if (stepSize < std::abs(angle))
        {
            auto steps = floor(std::abs(angle) / stepSize);
            stepSize = angle < 0 ? stepSize : -stepSize;

            for (auto i = 0; i < steps; ++i)
            {
                auto curAngle = i * stepSize;
                auto pointOnCircle = Matrix3::getRotation(curAngle).transformPoint(startingPointOnCircle);
                pointOnCircle /= pointOnCircle.getLength();

                glVertex3d(pointOnCircle.x(), pointOnCircle.y(), 0);
            }
        }

        auto currentPointOnCircle = _rotator.getCurrentDirection() * _circleRadius;

        glVertex3d(currentPointOnCircle.x(), currentPointOnCircle.y(), 0);

        glEnd();

        glDisable(GL_BLEND);
    }

    // Enable point colours if required
    glEnableClientState(GL_COLOR_ARRAY);

    pointvertex_gl_array(&_renderableCircle.front());
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(_renderableCircle.size()));

    glDisableClientState(GL_COLOR_ARRAY);

    if (_selectableZ.isSelected())
    {
        glColor3f(0.75f, 0.75f, 0.75f);
        glRasterPos3dv(Vector3(0, 0, 0));

        _glFont->drawString(fmt::format("Rotate: {0:3.2f} degrees", static_cast<float>(c_RAD2DEGMULT * angle)));
    }

    glPopMatrix();
}

void TextureToolRotateManipulator::rotateSelected(const Vector2& pivot, double angle)
{
    // Construct the full rotation around the pivot point
    auto transform = Matrix3::getTranslation(-pivot);
    transform.premultiplyBy(Matrix3::getRotation(-angle));
    transform.premultiplyBy(Matrix3::getTranslation(pivot));

    GlobalTextureToolSelectionSystem().foreachSelectedNode([&](const textool::INode::Ptr& node)
    {
        node->revertTransformation();
        node->applyTransformToSelected(transform);
        return true;
    });
}

}
