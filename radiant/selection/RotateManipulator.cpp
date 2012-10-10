#include "RotateManipulator.h"
#include "Selectors.h"
#include "BestPoint.h"

template<typename remap_policy>
inline void draw_semicircle(const std::size_t segments, const float radius, VertexCb* vertices, remap_policy remap) {
  const double increment = c_pi / double(segments << 2);

  std::size_t count = 0;
  float x = radius;
  float y = 0;
  remap_policy::set(vertices[segments << 2].vertex, -radius, 0, 0);
  while(count < segments)
  {
    VertexCb* i = vertices + count;
    VertexCb* j = vertices + ((segments << 1) - (count + 1));

    VertexCb* k = i + (segments << 1);
    VertexCb* l = j + (segments << 1);

#if 0
    VertexCb* m = i + (segments << 2);
    VertexCb* n = j + (segments << 2);
    VertexCb* o = k + (segments << 2);
    VertexCb* p = l + (segments << 2);
#endif

    remap_policy::set(i->vertex, x,-y, 0);
    remap_policy::set(k->vertex,-y,-x, 0);
#if 0
    remap_policy::set(m->vertex,-x, y, 0);
    remap_policy::set(o->vertex, y, x, 0);
#endif

    ++count;

    {
      const double theta = increment * count;
      x = static_cast<float>(radius * cos(theta));
      y = static_cast<float>(radius * sin(theta));
    }

    remap_policy::set(j->vertex, y,-x, 0);
    remap_policy::set(l->vertex,-x,-y, 0);
#if 0
    remap_policy::set(n->vertex,-y, x, 0);
    remap_policy::set(p->vertex, x, y, 0);
#endif
  }
}

// Constructor
RotateManipulator::RotateManipulator(Rotatable& rotatable, std::size_t segments, float radius) :
    _rotateFree(rotatable),
    _rotateAxis(rotatable),
    _circleX((segments << 2) + 1),
    _circleY((segments << 2) + 1),
    _circleZ((segments << 2) + 1),
    _circleScreen(segments<<3),
    _circleSphere(segments<<3)
{
	draw_semicircle(segments, radius, &_circleX.front(), RemapYZX());
    draw_semicircle(segments, radius, &_circleY.front(), RemapZXY());
    draw_semicircle(segments, radius, &_circleZ.front(), RemapXYZ());

	draw_circle(segments, radius * 1.15f, &_circleScreen.front(), RemapXYZ());
    draw_circle(segments, radius, &_circleSphere.front(), RemapXYZ());
}

void RotateManipulator::UpdateColours()
{
    _circleX.setColour(colourSelected(g_colour_x, _selectableX.isSelected()));
    _circleY.setColour(colourSelected(g_colour_y, _selectableY.isSelected()));
    _circleZ.setColour(colourSelected(g_colour_z, _selectableZ.isSelected()));
    _circleScreen.setColour(colourSelected(Manipulator::COLOUR_SCREEN(), _selectableScreen.isSelected()));
    _circleSphere.setColour(colourSelected(Manipulator::COLOUR_SPHERE(), false));
}

void RotateManipulator::updateCircleTransforms()
{
    Vector3 localViewpoint(
		_pivot._worldSpace.getTransposed().transformDirection(_pivot._viewpointSpace.z().getVector3())
    );

    _circleX_visible = !g_vector3_axis_x.isEqual(localViewpoint, 1e-6);
    if(_circleX_visible)
    {
      _local2worldX = Matrix4::getIdentity();
      _local2worldX.y().getVector3() = g_vector3_axis_x.crossProduct(localViewpoint).getNormalised();
      _local2worldX.z().getVector3() = _local2worldX.x().getVector3().crossProduct(
        											_local2worldX.y().getVector3()).getNormalised();
	  _local2worldX.premultiplyBy(_pivot._worldSpace);
    }

	_circleY_visible = !g_vector3_axis_y.isEqual(localViewpoint, 1e-6);
    if(_circleY_visible)
    {
      _local2worldY = Matrix4::getIdentity();
      _local2worldY.z().getVector3() = g_vector3_axis_y.crossProduct(localViewpoint).getNormalised();
      _local2worldY.x().getVector3() = _local2worldY.y().getVector3().crossProduct(
      													_local2worldY.z().getVector3()).getNormalised();
      _local2worldY.premultiplyBy(_pivot._worldSpace);
    }

	_circleZ_visible = !g_vector3_axis_z.isEqual(localViewpoint, 1e-6);
    if(_circleZ_visible)
    {
      _local2worldZ = Matrix4::getIdentity();
      _local2worldZ.x().getVector3() = g_vector3_axis_z.crossProduct(localViewpoint).getNormalised();
      _local2worldZ.y().getVector3() = _local2worldZ.z().getVector3().crossProduct(
      												_local2worldZ.x().getVector3()).getNormalised();
	  _local2worldZ.premultiplyBy(_pivot._worldSpace);
    }
}

void RotateManipulator::render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());
    updateCircleTransforms();

    // temp hack
    UpdateColours();

    collector.SetState(_stateOuter, RenderableCollector::eWireframeOnly);
    collector.SetState(_stateOuter, RenderableCollector::eFullMaterials);

    collector.addRenderable(_circleScreen, _pivot._viewpointSpace);
    collector.addRenderable(_circleSphere, _pivot._viewpointSpace);

    if(_circleX_visible)
    {
      collector.addRenderable(_circleX, _local2worldX);
    }
    if(_circleY_visible)
    {
      collector.addRenderable(_circleY, _local2worldY);
    }
    if(_circleZ_visible)
    {
      collector.addRenderable(_circleZ, _local2worldZ);
    }
}

void RotateManipulator::testSelect(const render::View& view, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());
    updateCircleTransforms();

    SelectionPool selector;

    {
      {
        Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_local2worldX));

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, &_circleX.front(), _circleX.size(), best);
        selector.addSelectable(best, &_selectableX);
      }

      {
		  Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_local2worldY));

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, &_circleY.front(), _circleY.size(), best);
        selector.addSelectable(best, &_selectableY);
      }

      {
		  Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_local2worldZ));

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, &_circleZ.front(), _circleZ.size(), best);
        selector.addSelectable(best, &_selectableZ);
      }
    }

    {
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot._viewpointSpace));

      {
        SelectionIntersection best;
        LineLoop_BestPoint(local2view, &_circleScreen.front(), _circleScreen.size(), best);
        selector.addSelectable(best, &_selectableScreen);
      }

      {
        SelectionIntersection best;
        Circle_BestPoint(local2view, eClipCullCW, &_circleSphere.front(), _circleSphere.size(), best);
        selector.addSelectable(best, &_selectableSphere);
      }
    }

    _axisScreen = _pivot._axisScreen;

    if(!selector.failed())
    {
      (*selector.begin()).second->setSelected(true);
    }
}

ManipulatorComponent* RotateManipulator::getActiveComponent() {
    if(_selectableX.isSelected()) {
      _rotateAxis.SetAxis(g_vector3_axis_x);
      return &_rotateAxis;
    }
    else if(_selectableY.isSelected()) {
      _rotateAxis.SetAxis(g_vector3_axis_y);
      return &_rotateAxis;
    }
    else if(_selectableZ.isSelected()) {
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

void RotateManipulator::setSelected(bool select)  {
    _selectableX.setSelected(select);
    _selectableY.setSelected(select);
    _selectableZ.setSelected(select);
    _selectableScreen.setSelected(select);
}

bool RotateManipulator::isSelected() const {
    return _selectableX.isSelected()
      | _selectableY.isSelected()
      | _selectableZ.isSelected()
      | _selectableScreen.isSelected()
      | _selectableSphere.isSelected();
}

// Initialise the shader of the RotateManipulator class
ShaderPtr RotateManipulator::_stateOuter;


