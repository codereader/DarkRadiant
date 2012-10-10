#include "TranslateManipulator.h"
#include "Remap.h"
#include "Selectors.h"
#include "BestPoint.h"

#include "registry/registry.h"

const std::string RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";

// Constructor
TranslateManipulator::TranslateManipulator(Translatable& translatable, std::size_t segments, float length) :
    _translateFree(translatable),
    _translateAxis(translatable),
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
    _arrowX.setColour(colourSelected(g_colour_x, _selectableX.isSelected()));
    _arrowHeadX.setColour(colourSelected(g_colour_x, _selectableX.isSelected()));
    _arrowY.setColour(colourSelected(g_colour_y, _selectableY.isSelected()));
    _arrowHeadY.setColour(colourSelected(g_colour_y, _selectableY.isSelected()));
    _arrowZ.setColour(colourSelected(g_colour_z, _selectableZ.isSelected()));
    _arrowHeadZ.setColour(colourSelected(g_colour_z, _selectableZ.isSelected()));
    _quadScreen.setColour(colourSelected(Manipulator::COLOUR_SCREEN(), _selectableScreen.isSelected()));
}

bool TranslateManipulator::manipulator_show_axis(const Pivot2World& pivot, const Vector3& axis) {
    return fabs(pivot._axisScreen.dot(axis)) < 0.95;
}

void TranslateManipulator::render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    // temp hack
    UpdateColours();

    Vector3 x = _pivot._worldSpace.x().getVector3().getNormalised();
    bool show_x = manipulator_show_axis(_pivot, x);

    Vector3 y = _pivot._worldSpace.y().getVector3().getNormalised();
    bool show_y = manipulator_show_axis(_pivot, y);

    Vector3 z = _pivot._worldSpace.z().getVector3().getNormalised();
    bool show_z = manipulator_show_axis(_pivot, z);

    collector.SetState(_stateWire, RenderableCollector::eWireframeOnly);
    collector.SetState(_stateWire, RenderableCollector::eFullMaterials);

    if(show_x)
    {
      collector.addRenderable(_arrowX, _pivot._worldSpace);
    }
    if(show_y)
    {
      collector.addRenderable(_arrowY, _pivot._worldSpace);
    }
    if(show_z)
    {
      collector.addRenderable(_arrowZ, _pivot._worldSpace);
    }

    collector.addRenderable(_quadScreen, _pivot._viewplaneSpace);

    collector.SetState(_stateFill, RenderableCollector::eWireframeOnly);
    collector.SetState(_stateFill, RenderableCollector::eFullMaterials);

    if(show_x)
    {
      collector.addRenderable(_arrowHeadX, _pivot._worldSpace);
    }
    if(show_y)
    {
      collector.addRenderable(_arrowHeadY, _pivot._worldSpace);
    }
    if(show_z)
    {
      collector.addRenderable(_arrowHeadZ, _pivot._worldSpace);
    }
}

void TranslateManipulator::testSelect(const render::View& view, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    Vector3 x = _pivot._worldSpace.x().getVector3().getNormalised();
    bool show_x = manipulator_show_axis(_pivot, x);

    Vector3 y = _pivot._worldSpace.y().getVector3().getNormalised();
    bool show_y = manipulator_show_axis(_pivot, y);

    Vector3 z = _pivot._worldSpace.z().getVector3().getNormalised();
    bool show_z = manipulator_show_axis(_pivot, z);

    {
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot._viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, &_quadScreen.front(), best);
        if(best.valid())
        {
          best = SelectionIntersection(0, 0);
          selector.addSelectable(best, &_selectableScreen);
        }
      }
    }

    {
		Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot._worldSpace));

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
    if(!selector.failed()) {
      (*selector.begin()).second->setSelected(true);
    } else {
    	Selectable* selectable = NULL;

    	if (registry::getValue<bool>(RKEY_TRANSLATE_CONSTRAINED)) {
	    	// None of the shown arrows (or quad) has been selected, select an axis based on the precedence
	    	Matrix4 local2view(view.GetViewMatrix().getMultipliedBy(_pivot._worldSpace));

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

ManipulatorComponent* TranslateManipulator::getActiveComponent() {
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

void TranslateManipulator::setSelected(bool select) {
    _selectableX.setSelected(select);
    _selectableY.setSelected(select);
    _selectableZ.setSelected(select);
    _selectableScreen.setSelected(select);
}

bool TranslateManipulator::isSelected() const {
    return _selectableX.isSelected()
      | _selectableY.isSelected()
      | _selectableZ.isSelected()
      | _selectableScreen.isSelected();
}

// Initialise the shaders of this class
ShaderPtr TranslateManipulator::_stateWire;
ShaderPtr TranslateManipulator::_stateFill;


