#include "TextureToolRotateManipulator.h"

#include "iselectiontest.h"
#include "itexturetoolmodel.h"
#include "itexturetoolcolours.h"
#include "ishaders.h"
#include "selection/BestPoint.h"
#include "selection/SelectionPool.h"
#include "pivot.h"
#include "math/Matrix3.h"
#include "registry/registry.h"

namespace textool
{

void TextureRotator::beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint)
{
    _deviceStart = devicePoint;

    auto device2Screen = view.GetViewport();
    auto screenStart3D = device2Screen.transformPoint(Vector3(_deviceStart.x(), _deviceStart.y(), 0));
    _screenStart = Vector2(screenStart3D.x(), screenStart3D.y());

    // Pivot in screen space
    auto pivot2Screen = constructPivot2Device(pivot2world, view).getPremultipliedBy(device2Screen);
    auto pivotInScreenSpace3D = pivot2Screen.transformPoint(Vector3(0, 0, 0));
    auto pivotInScreenSpace = Vector2(pivotInScreenSpace3D.x(), pivotInScreenSpace3D.y());

    // Screen start is relative to the pivot
    _screenStart -= pivotInScreenSpace;
    _screenStart.normalise();

    auto device2Pivot = constructDevice2Pivot(pivot2world, view);
    auto startRelativeToPivot = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));

    _start = Vector2(startRelativeToPivot.x(), startRelativeToPivot.y());
    _start.normalise();
}

void TextureRotator::transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int constraintFlags)
{
    _deviceCurrent = devicePoint;

    auto device2Screen = view.GetViewport();

    auto screenCurrent3D = device2Screen.transformPoint(Vector3(_deviceCurrent.x(), _deviceCurrent.y(), 0));
    _screenCurrent = Vector2(screenCurrent3D.x(), screenCurrent3D.y());

    // Get the pivot in screen space
    auto pivot2Screen = constructPivot2Device(pivot2world, view).getPremultipliedBy(device2Screen);

    auto pivotInScreenSpace3D = pivot2Screen.transformPoint(Vector3(0, 0, 0));
    auto pivotInScreenSpace = Vector2(pivotInScreenSpace3D.x(), pivotInScreenSpace3D.y());

    // Get the angle the mouse is currently drawing
    auto currentVec = (_screenCurrent - pivotInScreenSpace).getNormalised();

    _curAngle = acos(_screenStart.dot(currentVec));

    auto device2Pivot = constructDevice2Pivot(pivot2world, view);
    auto current3D = device2Pivot.transformPoint(Vector3(devicePoint.x(), devicePoint.y(), 0));
    _current = Vector2(current3D.x(), current3D.y());
    _current.normalise();

    if (constraintFlags & Constraint::Type1)
    {
        _curAngle = float_snapped(_curAngle, 5 * c_DEG2RADMULT);
    }

    auto sign = _screenStart.crossProduct(currentVec) < 0 ? +1 : -1;
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

const Vector2& TextureRotator::getStartDirectionInScreenSpace() const
{
    return _screenStart;
}

constexpr std::size_t CircleSegments = 8;
constexpr double DefaultCircleRadius= 150; // Is measured in device pixels, will be scaled back to UV space on the fly
constexpr double DefaultCrossHairSize = 10; // in device pixels

TextureToolRotateManipulator::TextureToolRotateManipulator(TextureToolManipulationPivot& pivot) :
    _pivot(pivot),
    _rotator(std::bind(&TextureToolRotateManipulator::rotateSelected, this, std::placeholders::_1, std::placeholders::_2)),
    _renderableCircle(GL_LINE_LOOP, CircleSegments << 3),
    _circleRadius(DefaultCircleRadius)
{
    draw_ellipse<RemapXYZ>(CircleSegments, static_cast<float>(DefaultCircleRadius), static_cast<float>(DefaultCircleRadius), _renderableCircle);
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
        selector.addWithNullIntersection(_selectableZ);
    }

    if (!selector.empty())
    {
        selector.begin()->second->setSelected(true);
    }
}

void TextureToolRotateManipulator::renderComponents(const render::IRenderView& view, const Matrix4& pivot2World)
{
    if (!_shader)
    {
        _shader = GlobalRenderSystem().capture(BuiltInShaderType::WireframeOverlay);

        auto manipulatorFontStyle = registry::getValue<std::string>(selection::RKEY_MANIPULATOR_FONTSTYLE) == "Sans" ?
            IGLFont::Style::Sans : IGLFont::Style::Mono;
        auto manipulatorFontSize = registry::getValue<int>(selection::RKEY_MANIPULATOR_FONTSIZE);

        _glFont = GlobalOpenGL().getFont(manipulatorFontStyle, manipulatorFontSize);
    }

    if (_renderableCircle.empty()) return;

    const auto& translation = pivot2World.tCol().getVector3();

    // Transform the ellipse radii back to UV space
    auto viewTransform = view.GetViewProjection().getPremultipliedBy(view.GetViewport());
    auto inverseView = viewTransform.getFullInverse();
    auto transformedDeviceUnit = inverseView.transformDirection(Vector3(1, 1, 0));

    // Recalculate the circle radius based on the view
    // Raw circle is R=DefaultCircleRadius centered around origin
    draw_ellipse<RemapXYZ>(CircleSegments, DefaultCircleRadius, DefaultCircleRadius, _renderableCircle);

    auto deselectedColour = GlobalTextureToolColourSchemeManager().getColour(SchemeElement::Manipulator);
    auto selectedColour = GlobalTextureToolColourSchemeManager().getColour(SchemeElement::SelectedManipulator);

    auto colour = isSelected() ? selectedColour : deselectedColour;
    Colour4b byteColour(static_cast<unsigned char>(colour.x() * 255),
        static_cast<unsigned char>(colour.y() * 255),
        static_cast<unsigned char>(colour.z() * 255),
        static_cast<unsigned char>(colour.w() * 255));
    _renderableCircle.setColour(byteColour);

    // Move the raw circle vertices to pivot space for rendering
    auto screen2Pivot = constructPivot2Device(pivot2World, view).getMultipliedBy(view.GetViewport()).getFullInverse();

    for (auto i = 0; i < _renderableCircle.size(); ++i)
    {
        _renderableCircle[i].vertex = screen2Pivot.transformPoint(_renderableCircle[i].vertex);
    }

    // Set up the model view such that we can render vertices relative to the pivot
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslated(translation.x(), translation.y(), 0);

    auto angle = _rotator.getCurAngle();

    // Crosshair
    glColor3fv(deselectedColour);
    glBegin(GL_LINES);

    auto crossHairSize = transformedDeviceUnit * DefaultCrossHairSize;
    auto crossHairAngle = _selectableZ.isSelected() ? -angle : 0;
    glVertex2d(cos(crossHairAngle) * crossHairSize.x(), sin(crossHairAngle) * crossHairSize.y());
    glVertex2d(-cos(crossHairAngle) * crossHairSize.x(), -sin(crossHairAngle) * crossHairSize.y());

    glVertex2d(cos(crossHairAngle + c_half_pi) * crossHairSize.x(), sin(crossHairAngle + c_half_pi) * crossHairSize.y());
    glVertex2d(-cos(crossHairAngle + c_half_pi) * crossHairSize.x(), -sin(crossHairAngle + c_half_pi) * crossHairSize.y());

    glEnd();

    if (_selectableZ.isSelected())
    {
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.3f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        auto surfaceColour = GlobalTextureToolColourSchemeManager().getColour(SchemeElement::ManipulatorSurface);
        glColor3fv(surfaceColour);

        glBegin(GL_TRIANGLE_FAN);

        glVertex3d(0, 0, 0);

        // Construct the circle sector in screen space coords, then transform it to pivot space
        auto startDirection = _rotator.getStartDirectionInScreenSpace();
        Vector2 startingPointOnCircle = startDirection * DefaultCircleRadius;

        glVertex3dv(screen2Pivot.transformPoint(Vector3(startingPointOnCircle.x(), startingPointOnCircle.y(), 0)));

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

                glVertex3dv(screen2Pivot.transformPoint(Vector3(pointOnCircle.x(), pointOnCircle.y(), 0)));
            }
        }

        auto currentPointOnCircle = Matrix3::getRotation(-angle).transformPoint(startingPointOnCircle);
        glVertex3dv(screen2Pivot.transformPoint(Vector3(currentPointOnCircle.x(), currentPointOnCircle.y(), 0)));

        glEnd();

        glDisable(GL_BLEND);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    pointvertex_gl_array(&_renderableCircle.front());
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(_renderableCircle.size()));

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (_selectableZ.isSelected())
    {
        glColor3fv(deselectedColour);

        auto transformedOffset = inverseView.transformDirection(Vector3(0.02, 0.02, 0));
        glRasterPos3dv(transformedOffset);

        _glFont->drawString(fmt::format("Rotate: {0:3.2f} degrees", static_cast<float>(c_RAD2DEGMULT * angle)));
    }

    glPopMatrix();
}

void TextureToolRotateManipulator::rotateSelected(const Vector2& pivot, double angle)
{
    // Check the aspect ratio of the active material
    auto material = GlobalMaterialManager().getMaterial(GlobalTextureToolSceneGraph().getActiveMaterial());
    auto texture = material->getEditorImage();
    auto aspectRatio = static_cast<float>(texture->getWidth()) / texture->getHeight();

    // Construct the full rotation around the pivot point
    auto transform = Matrix3::getTranslation(-pivot);
    transform.premultiplyBy(Matrix3::getScale({ aspectRatio, 1 }));
    transform.premultiplyBy(Matrix3::getRotation(angle));
    transform.premultiplyBy(Matrix3::getScale({ 1 / aspectRatio, 1 }));
    transform.premultiplyBy(Matrix3::getTranslation(pivot));

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
