#include "TranslateManipulator.h"

#include "../Remap.h"
#include "../SelectionPool.h"
#include "../BestPoint.h"
#include "render/View.h"

#include "registry/registry.h"

namespace selection
{

const std::string RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";

// Constructor
TranslateManipulator::TranslateManipulator(ManipulationPivot& pivot, std::size_t segments, float length) :
	_pivot(pivot),
	_translator(std::bind(&ManipulationPivot::applyTranslation, &_pivot, std::placeholders::_1)),
    _translateFree(_translator),
    _translateAxis(_translator),
    _arrowHeadX(3 * 2 * (segments << 3)),
    _arrowHeadY(3 * 2 * (segments << 3)),
    _arrowHeadZ(3 * 2 * (segments << 3))
{
    draw_arrowline(length, &_arrowX.front(), 0);
    draw_arrowhead(segments, length, &_arrowHeadX._vertices.front(), TripleRemapXYZ<Vertex3f>(), TripleRemapXYZ<Normal3f>());
    draw_arrowline(length, &_arrowY.front(), 1);
    draw_arrowhead(segments, length, &_arrowHeadY._vertices.front(), TripleRemapYZX<Vertex3f>(), TripleRemapYZX<Normal3f>());
    draw_arrowline(length, &_arrowZ.front(), 2);
    draw_arrowhead(segments, length, &_arrowHeadZ._vertices.front(), TripleRemapZXY<Vertex3f>(), TripleRemapZXY<Normal3f>());

    draw_quad(16, &_quadScreen.front());
}

void TranslateManipulator::UpdateColours() {
    _arrowX.setColour(colourSelected(COLOUR_X(), _selectableX.isSelected()));
    _arrowHeadX.setColour(colourSelected(COLOUR_X(), _selectableX.isSelected()));
    _arrowY.setColour(colourSelected(COLOUR_Y(), _selectableY.isSelected()));
    _arrowHeadY.setColour(colourSelected(COLOUR_Y(), _selectableY.isSelected()));
    _arrowZ.setColour(colourSelected(COLOUR_Z(), _selectableZ.isSelected()));
    _arrowHeadZ.setColour(colourSelected(COLOUR_Z(), _selectableZ.isSelected()));
    _quadScreen.setColour(colourSelected(ManipulatorBase::COLOUR_SCREEN(), _selectableScreen.isSelected()));
}

bool TranslateManipulator::manipulator_show_axis(const Pivot2World& pivot, const Vector3& axis) {
    return fabs(pivot._axisScreen.dot(axis)) < 0.95;
}

void TranslateManipulator::render(RenderableCollector& collector, const VolumeTest& volume)
{
    _pivot2World.update(_pivot.getMatrix4(), volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    // temp hack
    UpdateColours();

    Vector3 x = _pivot2World._worldSpace.x().getVector3().getNormalised();
    bool show_x = manipulator_show_axis(_pivot2World, x);

    Vector3 y = _pivot2World._worldSpace.y().getVector3().getNormalised();
    bool show_y = manipulator_show_axis(_pivot2World, y);

    Vector3 z = _pivot2World._worldSpace.z().getVector3().getNormalised();
    bool show_z = manipulator_show_axis(_pivot2World, z);

    if(show_x)
    {
      collector.addRenderable(_stateWire, _arrowX, _pivot2World._worldSpace);
    }
    if(show_y)
    {
      collector.addRenderable(_stateWire, _arrowY, _pivot2World._worldSpace);
    }
    if(show_z)
    {
      collector.addRenderable(_stateWire, _arrowZ, _pivot2World._worldSpace);
    }

    collector.addRenderable(_stateWire, _quadScreen, _pivot2World._viewplaneSpace);

    if(show_x)
    {
      collector.addRenderable(_stateFill, _arrowHeadX, _pivot2World._worldSpace);
    }
    if(show_y)
    {
      collector.addRenderable(_stateFill, _arrowHeadY, _pivot2World._worldSpace);
    }
    if(show_z)
    {
      collector.addRenderable(_stateFill, _arrowHeadZ, _pivot2World._worldSpace);
    }
}

void TranslateManipulator::testSelect(const render::View& view, const Matrix4& pivot2world)
{
    _pivot2World.update(_pivot.getMatrix4(), view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    Vector3 x = _pivot2World._worldSpace.x().getVector3().getNormalised();
    bool show_x = manipulator_show_axis(_pivot2World, x);

    Vector3 y = _pivot2World._worldSpace.y().getVector3().getNormalised();
    bool show_y = manipulator_show_axis(_pivot2World, y);

    Vector3 z = _pivot2World._worldSpace.z().getVector3().getNormalised();
    bool show_z = manipulator_show_axis(_pivot2World, z);

    {
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, &_quadScreen.front(), best);
        if(best.isValid())
        {
          best = SelectionIntersection(0, 0);
          selector.addSelectable(best, &_selectableScreen);
        }
      }
    }

    {
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._worldSpace));

      if(show_x)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowX.front(), best);
        Triangles_BestPoint(local2view, eClipCullCW, &_arrowHeadX._vertices.front(), &*(_arrowHeadX._vertices.end()-1)+1, best);
        selector.addSelectable(best, &_selectableX);
      }

      if(show_y)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowY.front(), best);
        Triangles_BestPoint(local2view, eClipCullCW, &_arrowHeadY._vertices.front(), &*(_arrowHeadY._vertices.end()-1)+1, best);
        selector.addSelectable(best, &_selectableY);
      }

      if(show_z)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowZ.front(), best);
        Triangles_BestPoint(local2view, eClipCullCW, &_arrowHeadZ._vertices.front(), &*(_arrowHeadZ._vertices.end()-1)+1, best);
        selector.addSelectable(best, &_selectableZ);
      }
    }

	// greebo: If any of the above arrows could be selected, select the first in the SelectionPool
    if(!selector.empty()) {
      (*selector.begin()).second->setSelected(true);
    } else {
    	ISelectable* selectable = NULL;

    	if (registry::getValue<bool>(RKEY_TRANSLATE_CONSTRAINED)) {
	    	// None of the shown arrows (or quad) has been selected, select an axis based on the precedence
	    	Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot2World._worldSpace));

	    	// Get the (relative?) distance from the mouse pointer to the manipulator
	    	Vector3 delta = local2view.t().getProjected();

	    	// Get the precedence (which axis has the absolute largest value in it)
	    	bool xGreaterY = (fabs(delta.x()) > fabs(delta.y()));

	    	// The precedence has to be interpreted according to which axes are visible
	    	if (show_z) {
	    		// Either XZ or YZ
	    		if (show_y) {
	    			// YZ
	    			selectable = (xGreaterY) ? &_selectableY : &_selectableZ;
	    		}
	    		else {
	    			// XZ
	    			selectable = (xGreaterY) ? &_selectableX : &_selectableZ;
	    		}
	    	}
	    	else {
	    		// XY
	    		selectable = (xGreaterY) ? &_selectableX : &_selectableY;
	    	}
    	}
    	else {
    		// Don't constrain to axis, choose the freemove translator
    		selectable = &_selectableScreen;
    	}

		// If everything went ok, there is a selectable available, add it
    	if (selectable != NULL) {
    		selector.addSelectable(SelectionIntersection(0,0), selectable);
    		selectable->setSelected(true);
    	}
    }
}

TranslateManipulator::Component* TranslateManipulator::getActiveComponent()
{
    if(_selectableX.isSelected())
    {
      _translateAxis.SetAxis(g_vector3_axis_x);
      return &_translateAxis;
    }
    else if(_selectableY.isSelected())
    {
      _translateAxis.SetAxis(g_vector3_axis_y);
      return &_translateAxis;
    }
    else if(_selectableZ.isSelected())
    {
      _translateAxis.SetAxis(g_vector3_axis_z);
      return &_translateAxis;
    }
    else
    {
      return &_translateFree;
    }
}

void TranslateManipulator::setSelected(bool select)
{
    _selectableX.setSelected(select);
    _selectableY.setSelected(select);
    _selectableZ.setSelected(select);
    _selectableScreen.setSelected(select);
}

bool TranslateManipulator::isSelected() const
{
    return _selectableX.isSelected()
      | _selectableY.isSelected()
      | _selectableZ.isSelected()
      | _selectableScreen.isSelected();
}

// Initialise the shaders of this class
ShaderPtr TranslateManipulator::_stateWire;
ShaderPtr TranslateManipulator::_stateFill;

}
