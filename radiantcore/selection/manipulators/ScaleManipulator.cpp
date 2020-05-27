#include "ScaleManipulator.h"

#include "selection/Remap.h"
#include "selection/SelectionPool.h"
#include "selection/BestPoint.h"
#include "selection/TransformationVisitors.h"
#include "render/View.h"

namespace selection
{

// Constructor
ScaleManipulator::ScaleManipulator(ManipulationPivot& pivot, std::size_t segments, float length) :
	_pivot(pivot),
    _scaleFree(*this),
    _scaleAxis(*this)
{
    draw_arrowline(length, &_arrowX.front(), 0);
    draw_arrowline(length, &_arrowY.front(), 1);
    draw_arrowline(length, &_arrowZ.front(), 2);

    draw_quad(16, &_quadScreen.front());
}

void ScaleManipulator::UpdateColours() {
    _arrowX.setColour(colourSelected(COLOUR_X(), _selectableX.isSelected()));
    _arrowY.setColour(colourSelected(COLOUR_Y(), _selectableY.isSelected()));
    _arrowZ.setColour(colourSelected(COLOUR_Z(), _selectableZ.isSelected()));
    _quadScreen.setColour(colourSelected(ManipulatorBase::COLOUR_SCREEN(), _selectableScreen.isSelected()));
}

void ScaleManipulator::render(RenderableCollector& collector, const VolumeTest& volume)
{
    _pivot2World.update(_pivot.getMatrix4(), volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    // temp hack
    UpdateColours();

	// greebo: Deactivated the render functions for now, as there's no shader acquired in this manipulator
#if 0
    collector.addRenderable(_arrowX, _pivot2World._worldSpace);
    collector.addRenderable(_arrowY, _pivot2World._worldSpace);
    collector.addRenderable(_arrowZ, _pivot2World._worldSpace);

    collector.addRenderable(_quadScreen, _pivot2World._viewpointSpace);
#endif
}

void ScaleManipulator::testSelect(const render::View& view, const Matrix4& pivot2world)
{
    _pivot2World.update(_pivot.getMatrix4(), view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    {
      Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._worldSpace));

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
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, &_quadScreen.front(), best);
        selector.addSelectable(best, &_selectableScreen);
      }
    }

    if(!selector.empty())
    {
      (*selector.begin()).second->setSelected(true);
    }
}

ScaleManipulator::Component* ScaleManipulator::getActiveComponent()
{
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

void ScaleManipulator::setSelected(bool select) 
{
    _selectableX.setSelected(select);
    _selectableY.setSelected(select);
    _selectableZ.setSelected(select);
    _selectableScreen.setSelected(select);
}

bool ScaleManipulator::isSelected() const
{
    return _selectableX.isSelected()
      | _selectableY.isSelected()
      | _selectableZ.isSelected()
      | _selectableScreen.isSelected();
}

void ScaleManipulator::scale(const Vector3& scaling)
{
	// Pass the scale to the according traversor
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
	{
		GlobalSelectionSystem().foreachSelectedComponent(ScaleComponentSelected(scaling, _pivot.getVector3()));
	}
	else
	{
		GlobalSelectionSystem().foreachSelected(ScaleSelected(scaling, _pivot.getVector3()));
	}

	// Update the scene views
	SceneChangeNotify();
}

}
