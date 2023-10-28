#include "RotateManipulator.h"

#include "igl.h"
#include "selection/SelectionPool.h"
#include "selection/BestPoint.h"
#include "selection/TransformationVisitors.h"
#include "registry/registry.h"
#include <fmt/format.h>

namespace selection
{

namespace
{
    constexpr static auto CircleSegments = 8;
    constexpr static auto CircleRadius = 64.0;
    const Vector4 AngleTextColour(0.75, 0, 0, 1);

    static const Matrix4 REMAP_YZX = Matrix4::byRows(
        0, 0, 1, 0,
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 0, 1
    );
    static const Matrix4 REMAP_ZXY = Matrix4::byRows(
        0, 1, 0, 0,
        0, 0, 1, 0,
        1, 0, 0, 0,
        0, 0, 0, 1
    );
}

RotateManipulator::RotateManipulator(ManipulationPivot& pivot, std::size_t segments, float radius)
: _pivot(pivot),
  _pivotTranslatable(_pivot),
  _rotateFree(*this),
  _rotateAxis(*this),
  _translatePivot(_pivotTranslatable),
  _localPivotPoint(0, 0, 0),
  _circle{
      {CircleSegments, CircleRadius, _local2world.x, REMAP_YZX},
      {CircleSegments, CircleRadius, _local2world.y, REMAP_ZXY},
      {CircleSegments, CircleRadius, _local2world.z, Matrix4::getIdentity()}
  },
  _circleScreen(CircleSegments, CircleRadius * 1.15, _pivot2World._viewpointSpace),
  _circleSphere(CircleSegments, CircleRadius, _pivot2World._viewpointSpace),
  _pivotPoint(_localPivotPoint, _pivot2World._worldSpace),
  _angleText("", {0, 0, 0}, AngleTextColour)
{}

void RotateManipulator::updateColours()
{
    _circle.x.setColour(colourSelected(COLOUR_X(), _selectable.x.isSelected()));
    _circle.y.setColour(colourSelected(COLOUR_Y(), _selectable.y.isSelected()));
    _circle.z.setColour(colourSelected(COLOUR_Z(), _selectable.z.isSelected()));
    _circleScreen.setColour(colourSelected(COLOUR_SCREEN(), _selectableScreen.isSelected()));
	_pivotPoint.setColour(colourSelected(COLOUR_SPHERE(), _selectablePivotPoint.isSelected()));
}

void RotateManipulator::updateCircleTransforms()
{
    Vector3 localViewpoint(
        _pivot2World._worldSpace.getTransposed().transformDirection(
            _pivot2World._viewpointSpace.zCol3()));

    const bool circleXVisible = !math::isNear(g_vector3_axis_x, localViewpoint, 1e-6);
    if (circleXVisible)
    {
        _local2world.x = Matrix4::getIdentity();
        _local2world.x.setYCol(
            g_vector3_axis_x.cross(localViewpoint).getNormalised()
        );
        _local2world.x.setZCol(
            _local2world.x.xCol3().cross(_local2world.x.yCol3()).getNormalised()
        );
        _local2world.x.premultiplyBy(_pivot2World._worldSpace);
    }

    const bool circleYVisible = !math::isNear(g_vector3_axis_y, localViewpoint, 1e-6);
    if (circleYVisible)
    {
        _local2world.y = Matrix4::getIdentity();
        _local2world.y.setZCol(
            g_vector3_axis_y.cross(localViewpoint).getNormalised()
        );
        _local2world.y.setXCol(
            _local2world.y.yCol3().cross(_local2world.y.zCol3()).getNormalised()
        );
        _local2world.y.premultiplyBy(_pivot2World._worldSpace);
    }

    const bool circleZVisible = !math::isNear(g_vector3_axis_z, localViewpoint, 1e-6);
    if (circleZVisible)
    {
        _local2world.z = Matrix4::getIdentity();
        _local2world.z.setXCol(
            g_vector3_axis_z.cross(localViewpoint).getNormalised()
        );
        _local2world.z.setYCol(
            _local2world.z.zCol3().cross(_local2world.z.xCol3()).getNormalised()
        );
        _local2world.z.premultiplyBy(_pivot2World._worldSpace);
    }
}

void RotateManipulator::onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume)
{
    if (!renderSystem)
    {
        clearRenderables();
        return;
    }

    if (!_lineShader)
    {
        _lineShader = renderSystem->capture(BuiltInShaderType::ManipulatorWireframe);
    }

    if (!_pivotPointShader)
    {
        _pivotPointShader = renderSystem->capture(BuiltInShaderType::BigPoint);
    }

    if (!_textRenderer)
    {
        auto fontStyle = registry::getValue<std::string>(RKEY_MANIPULATOR_FONTSTYLE) == "Sans" ?
            IGLFont::Style::Sans : IGLFont::Style::Mono;
        auto fontSize = registry::getValue<int>(RKEY_MANIPULATOR_FONTSIZE);

        _textRenderer = renderSystem->captureTextRenderer(fontStyle, fontSize);
    }

    _pivot2World.update(_pivot.getMatrix4(), volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    updateCircleTransforms();
    updateColours();
    updateAngleText();

    _circle.x.update(_lineShader);
    _circle.y.update(_lineShader);
    _circle.z.update(_lineShader);
    _circleScreen.update(_lineShader);
    _pivotPoint.update(_pivotPointShader);
    _angleText.update(_textRenderer);
}

void RotateManipulator::clearRenderables()
{
    _circle.x.clear();
    _circle.y.clear();
    _circle.z.clear();
    _circleScreen.clear();
    _pivotPoint.clear();
    _angleText.clear();

    _lineShader.reset();
    _pivotPointShader.reset();
    _textRenderer.reset();
}

std::string RotateManipulator::getRotationAxisName() const
{
    if (_selectable.x.isSelected()) return "X";
    if (_selectable.y.isSelected()) return "Y";
    if (_selectable.z.isSelected()) return "Z";

    return std::string();
}

void RotateManipulator::updateAngleText()
{
    if (_selectable.x.isSelected() || _selectable.y.isSelected() ||
        _selectable.z.isSelected() || _selectableScreen.isSelected())
    {
        double angle = static_cast<double>(c_RAD2DEGMULT * _rotateAxis.getCurAngle());
        auto rotationAxisName = getRotationAxisName();

        _angleText.setText(fmt::format("Rotate: {0:3.2f} degrees {1}", angle, rotationAxisName));
        _angleText.setWorldPosition(_pivot2World._worldSpace.translation() - Vector3(10, 10, 10));
    }
    else
    {
        _angleText.setText("");
    }
}

void RotateManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
{
    _pivot2World.update(_pivot.getMatrix4(), test.getVolume().GetModelview(),
        test.getVolume().GetProjection(), test.getVolume().GetViewport());
    updateCircleTransforms();

    SelectionPool selector;

    if (test.getVolume().TestPoint(_pivot.getVector3()))
    {
        selector.addSelectable(SelectionIntersection(0, 0), &_selectablePivotPoint);
    }
    else
    {
        auto viewProj = test.getVolume().GetViewProjection();
        {
            Matrix4 local2view = viewProj * _local2world.x;

            SelectionIntersection best;
            LineStrip_BestPoint(local2view, &_circle.x.getRawPoints().front(),
                                _circle.x.getRawPoints().size(), best);
            selector.addSelectable(best, &_selectable.x);
        }

        {
            Matrix4 local2view = viewProj * _local2world.y;

            SelectionIntersection best;
            LineStrip_BestPoint(local2view, &_circle.y.getRawPoints().front(),
                                _circle.y.getRawPoints().size(), best);
            selector.addSelectable(best, &_selectable.y);
        }

        {
            Matrix4 local2view = viewProj * _local2world.z;

            SelectionIntersection best;
            LineStrip_BestPoint(local2view, &_circle.z.getRawPoints().front(),
                                _circle.z.getRawPoints().size(), best);
            selector.addSelectable(best, &_selectable.z);
        }

        {
            Matrix4 local2view = viewProj * _pivot2World._viewpointSpace;

            {
                SelectionIntersection best;
                const auto& points = _circleScreen.getRawPoints();
                LineLoop_BestPoint(local2view, &points.front(), points.size(), best);
                selector.addSelectable(best, &_selectableScreen);
            }

            {
                SelectionIntersection best;
                const auto& points = _circleSphere.getRawPoints();
                Circle_BestPoint(local2view, eClipCullCCW, &points.front(), points.size(), best);
                selector.addSelectable(best, &_selectableSphere);
            }
        }
    }

    _axisScreen = _pivot2World._axisScreen;

    if (!selector.empty())
        selector.begin()->second->setSelected(true);
}

RotateManipulator::Component* RotateManipulator::getActiveComponent()
{
	if (_selectablePivotPoint.isSelected())
	{
		return &_translatePivot;
	}

    if(_selectable.x.isSelected()) {
      _rotateAxis.SetAxis(g_vector3_axis_x);
      return &_rotateAxis;
    }
    else if(_selectable.y.isSelected()) {
      _rotateAxis.SetAxis(g_vector3_axis_y);
      return &_rotateAxis;
    }
    else if(_selectable.z.isSelected()) {
      _rotateAxis.SetAxis(g_vector3_axis_z);
      return &_rotateAxis;
    }
    else if(_selectableScreen.isSelected()) {
      _rotateAxis.SetAxis(_axisScreen);
      return &_rotateAxis;
    }
    else
      return &_rotateFree;
}

void RotateManipulator::setSelected(bool select)
{
    selection::setSelected(select, _selectable.x, _selectable.y, _selectable.z,
                           _selectableScreen, _selectablePivotPoint);

    if (!select) {
        _rotateAxis.resetCurAngle();
    }
}

bool RotateManipulator::isSelected() const
{
    return selection::isAnySelected(
        _selectable.x, _selectable.y, _selectable.z, _selectableScreen, _selectableSphere,
        _selectablePivotPoint
    );
}

void RotateManipulator::rotate(const Quaternion& rotation)
{
	// Perform the rotation according to the current mode
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component)
	{
		GlobalSelectionSystem().foreachSelectedComponent(
			RotateComponentSelected(rotation, _pivot.getVector3()));
	}
	else
	{
		// Cycle through the selections and rotate them
		GlobalSelectionSystem().foreachSelected(RotateSelected(rotation, _pivot.getVector3()));
	}

	SceneChangeNotify();
}
}
