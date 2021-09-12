#include "TextureToolRotateManipulator.h"

#include "iselectiontest.h"
#include "selection/BestPoint.h"
#include "selection/SelectionPool.h"

namespace selection
{

void TextureRotator::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{

}

void TextureRotator::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraints)
{

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
        //rMessage() << "Got the circle!" << std::endl;

        selector.begin()->second->setSelected(true);
    }
    else
    {
        //rMessage() << "MISSED" << std::endl;
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

}
