#include "TranslateManipulator.h"

#include "selection/SelectionPool.h"
#include "selection/BestPoint.h"

namespace selection
{

constexpr const char* const RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";

TranslateManipulator::TranslateManipulator(ManipulationPivot& pivot, std::size_t segments, float length) :
	_pivot(pivot),
	_translator(std::bind(&ManipulationPivot::applyTranslation, &_pivot, std::placeholders::_1)),
    _translateFree(_translator),
    _translateAxis(_translator),
    _arrowX({ length,0,0 }, _pivot2World._worldSpace),
    _arrowY({ 0,length,0 }, _pivot2World._worldSpace),
    _arrowZ({ 0,0,length }, _pivot2World._worldSpace),
    _arrowHeadX({ length,0,0 }, _pivot2World._axisScreen, length / 8, length / 3, _pivot2World._worldSpace),
    _arrowHeadY({ 0,length,0 }, _pivot2World._axisScreen, length / 8, length / 3, _pivot2World._worldSpace),
    _arrowHeadZ({ 0,0,length }, _pivot2World._axisScreen, length / 8, length / 3, _pivot2World._worldSpace),
    _quadScreen(16, _pivot2World._viewplaneSpace),
    _translateConstrained(RKEY_TRANSLATE_CONSTRAINED)
{}

void TranslateManipulator::updateColours()
{
    _arrowX.setColour(colourSelected(COLOUR_X(), _selectableX.isSelected()));
    _arrowHeadX.setColour(colourSelected(COLOUR_X(), _selectableX.isSelected()));
    _arrowY.setColour(colourSelected(COLOUR_Y(), _selectableY.isSelected()));
    _arrowHeadY.setColour(colourSelected(COLOUR_Y(), _selectableY.isSelected()));
    _arrowZ.setColour(colourSelected(COLOUR_Z(), _selectableZ.isSelected()));
    _arrowHeadZ.setColour(colourSelected(COLOUR_Z(), _selectableZ.isSelected()));
    _quadScreen.setColour(colourSelected(COLOUR_SCREEN(), _selectableScreen.isSelected()));
}

bool TranslateManipulator::axisIsVisible(const Vector3& axis) const
{
    return fabs(_pivot2World._axisScreen.dot(axis)) < 0.95;
}

void TranslateManipulator::onPreRender(const RenderSystemPtr& renderSystem, const VolumeTest& volume)
{
    if (!renderSystem)
    {
        clearRenderables();
        return;
    }

    if (!_lineShader)
    {
        _lineShader = renderSystem->capture(BuiltInShaderType::WireframeOverlay);
    }

    if (!_arrowHeadShader)
    {
        _arrowHeadShader = renderSystem->capture(BuiltInShaderType::FlatshadeOverlay);
    }

    _pivot2World.update(_pivot.getMatrix4(), volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    updateColours();

    auto x = _pivot2World._worldSpace.xCol3().getNormalised();
    auto y = _pivot2World._worldSpace.yCol3().getNormalised();
    auto z = _pivot2World._worldSpace.zCol3().getNormalised();

    if (axisIsVisible(x))
    {
        _arrowX.update(_lineShader);
        _arrowHeadX.update(_arrowHeadShader);
    }
    else
    {
        _arrowX.clear();
        _arrowHeadX.clear();
    }

    if (axisIsVisible(y))
    {
        _arrowY.update(_lineShader);
        _arrowHeadY.update(_arrowHeadShader);
    }
    else
    {
        _arrowY.clear();
        _arrowHeadY.clear();
    }

    if (axisIsVisible(z))
    {
        _arrowZ.update(_lineShader);
        _arrowHeadZ.update(_arrowHeadShader);
    }
    else
    {
        _arrowZ.clear();
        _arrowHeadZ.clear();
    }

    _quadScreen.update(_lineShader);
}

void TranslateManipulator::clearRenderables()
{
    _arrowX.clear();
    _arrowY.clear();
    _arrowZ.clear();
    _arrowHeadX.clear();
    _arrowHeadY.clear();
    _arrowHeadZ.clear();
    _quadScreen.clear();

    _lineShader.reset();
    _arrowHeadShader.reset();
}

void TranslateManipulator::testSelect(SelectionTest& test, const Matrix4& pivot2world)
{
    _pivot2World.update(_pivot.getMatrix4(), test.getVolume().GetModelview(),
        test.getVolume().GetProjection(), test.getVolume().GetViewport());

    SelectionPool selector;

    auto x = _pivot2World._worldSpace.xCol3().getNormalised();
    auto y = _pivot2World._worldSpace.yCol3().getNormalised();
    auto z = _pivot2World._worldSpace.zCol3().getNormalised();

    bool show_x = axisIsVisible(x);
    bool show_y = axisIsVisible(y);
    bool show_z = axisIsVisible(z);

    {
		Matrix4 local2view(test.getVolume().GetViewProjection().getMultipliedBy(_pivot2World._viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, &_quadScreen.getRawPoints().front(), best);
        if(best.isValid())
        {
          best = SelectionIntersection(0, 0);
          selector.addSelectable(best, &_selectableScreen);
        }
      }
    }

    {
		Matrix4 local2view(test.getVolume().GetViewProjection().getMultipliedBy(_pivot2World._worldSpace));

      if(show_x)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowX.getRawPoints().front(), best);
        Triangles_BestPoint(local2view, eClipCullCW, &_arrowHeadX.getRawPoints().front(), &*(_arrowHeadX.getRawPoints().end()-1)+1, best);
        selector.addSelectable(best, &_selectableX);
      }

      if(show_y)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowY.getRawPoints().front(), best);
        Triangles_BestPoint(local2view, eClipCullCW, &_arrowHeadY.getRawPoints().front(), &*(_arrowHeadY.getRawPoints().end()-1)+1, best);
        selector.addSelectable(best, &_selectableY);
      }

      if(show_z)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, &_arrowZ.getRawPoints().front(), best);
        Triangles_BestPoint(local2view, eClipCullCW, &_arrowHeadZ.getRawPoints().front(), &*(_arrowHeadZ.getRawPoints().end()-1)+1, best);
        selector.addSelectable(best, &_selectableZ);
      }
    }

	// greebo: If any of the above arrows could be selected, select the first in the SelectionPool
    if(!selector.empty())
    {
        selector.begin()->second->setSelected(true);
    }
    else
    {
    	ISelectable* selectable = nullptr;

    	if (_translateConstrained.get())
        {
	    	// None of the shown arrows (or quad) has been selected, select an axis based on the precedence
	    	Matrix4 local2view(test.getVolume().GetViewProjection().getMultipliedBy(_pivot2World._worldSpace));

	    	// Get the (relative?) distance from the mouse pointer to the manipulator
	    	Vector3 delta = local2view.tCol().getProjected();

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
    	if (selectable)
        {
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
    return _selectableX.isSelected() || _selectableY.isSelected()
        || _selectableZ.isSelected() || _selectableScreen.isSelected();
}

}
