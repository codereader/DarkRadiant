#include "ScaleManipulator.h"
#include "Remap.h"
#include "Selectors.h"
#include "BestPoint.h"

// Constructor
ScaleManipulator::ScaleManipulator(Scalable& scalable, std::size_t segments, float length) :
    _scaleFree(scalable),
    _scaleAxis(scalable)
{
    draw_arrowline(length, &_arrowX.front(), 0);
    draw_arrowline(length, &_arrowY.front(), 1);
    draw_arrowline(length, &_arrowZ.front(), 2);

    draw_quad(16, &_quadScreen.front());
}

void ScaleManipulator::UpdateColours() {
    _arrowX.setColour(colourSelected(g_colour_x, _selectableX.isSelected()));
    _arrowY.setColour(colourSelected(g_colour_y, _selectableY.isSelected()));
    _arrowZ.setColour(colourSelected(g_colour_z, _selectableZ.isSelected()));
    _quadScreen.setColour(colourSelected(Manipulator::COLOUR_SCREEN(), _selectableScreen.isSelected()));
}

void ScaleManipulator::render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    // temp hack
    UpdateColours();

    collector.addRenderable(_arrowX, _pivot._worldSpace);
    collector.addRenderable(_arrowY, _pivot._worldSpace);
    collector.addRenderable(_arrowZ, _pivot._worldSpace);

    collector.addRenderable(_quadScreen, _pivot._viewpointSpace);
}

void ScaleManipulator::testSelect(const render::View& view, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    {
      Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot._worldSpace));

    {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowX.front(), best);
        selector.addSelectable(best, &_selectableX);
      }

      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowY.front(), best);
        selector.addSelectable(best, &_selectableY);
      }

      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowZ.front(), best);
        selector.addSelectable(best, &_selectableZ);
      }
    }

    {
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot._viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, &_quadScreen.front(), best);
        selector.addSelectable(best, &_selectableScreen);
      }
    }

    if(!selector.failed())
    {
      (*selector.begin()).second->setSelected(true);
    }
}

ManipulatorComponent* ScaleManipulator::getActiveComponent() {
    if(_selectableX.isSelected())
    {
      _scaleAxis.SetAxis(g_vector3_axis_x);
      return &_scaleAxis;
    }
    else if(_selectableY.isSelected())
    {
      _scaleAxis.SetAxis(g_vector3_axis_y);
      return &_scaleAxis;
    }
    else if(_selectableZ.isSelected())
    {
      _scaleAxis.SetAxis(g_vector3_axis_z);
      return &_scaleAxis;
    }
    else
      return &_scaleFree;
}

void ScaleManipulator::setSelected(bool select) {
    _selectableX.setSelected(select);
    _selectableY.setSelected(select);
    _selectableZ.setSelected(select);
    _selectableScreen.setSelected(select);
}

bool ScaleManipulator::isSelected() const {
    return _selectableX.isSelected()
      | _selectableY.isSelected()
      | _selectableZ.isSelected()
      | _selectableScreen.isSelected();
}


