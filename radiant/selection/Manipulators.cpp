#include "Manipulators.h"

#include "iregistry.h"

#include "Remap.h"
#include "Selectors.h"
#include "BestPoint.h"
#include "TransformationVisitors.h"
#include "SelectionTest.h"
#include "Planes.h"
#include "renderer.h"

namespace {
	const std::string RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";
	const std::string RKEY_TRANSIENT_COMPONENT_SELECTION = "user/ui/transientComponentSelection";
}

// ------------ Helper functions ---------------------------

const Colour4b g_colour_sphere(0, 0, 0, 255);
const Colour4b g_colour_screen(0, 255, 255, 255);
const Colour4b g_colour_selected(255, 255, 0, 255);

inline const Colour4b& colourSelected(const Colour4b& colour, bool selected) {
  return (selected) ? g_colour_selected : colour;
}

template<typename remap_policy>
inline void draw_semicircle(const std::size_t segments, const float radius, PointVertex* vertices, remap_policy remap) {
  const double increment = c_pi / double(segments << 2);

  std::size_t count = 0;
  float x = radius;
  float y = 0;
  remap_policy::set(vertices[segments << 2].vertex, -radius, 0, 0);
  while(count < segments)
  {
    PointVertex* i = vertices + count;
    PointVertex* j = vertices + ((segments << 1) - (count + 1));

    PointVertex* k = i + (segments << 1);
    PointVertex* l = j + (segments << 1);

#if 0
    PointVertex* m = i + (segments << 2);
    PointVertex* n = j + (segments << 2);
    PointVertex* o = k + (segments << 2);
    PointVertex* p = l + (segments << 2);
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

// ------------ RotateManipulator methods ------------------ 

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
    _circleScreen.setColour(colourSelected(g_colour_screen, _selectableScreen.isSelected()));
    _circleSphere.setColour(colourSelected(g_colour_sphere, false));
}
  
void RotateManipulator::updateCircleTransforms()  {
    Vector3 localViewpoint(
    	matrix4_transformed_direction(
    		_pivot._worldSpace.getTransposed(), 
    		_pivot._viewpointSpace.z().getVector3())
    );

    _circleX_visible = !vector3_equal_epsilon(g_vector3_axis_x, localViewpoint, 1e-6);
    if(_circleX_visible)
    {
      _local2worldX = Matrix4::getIdentity();
      _local2worldX.y().getVector3() = g_vector3_axis_x.crossProduct(localViewpoint).getNormalised();
      _local2worldX.z().getVector3() = _local2worldX.x().getVector3().crossProduct( 
        											_local2worldX.y().getVector3()).getNormalised();
      matrix4_premultiply_by_matrix4(_local2worldX, _pivot._worldSpace);
    }

    _circleY_visible = !vector3_equal_epsilon(g_vector3_axis_y, localViewpoint, 1e-6);
    if(_circleY_visible)
    {
      _local2worldY = Matrix4::getIdentity();
      _local2worldY.z().getVector3() = g_vector3_axis_y.crossProduct(localViewpoint).getNormalised();
      _local2worldY.x().getVector3() = _local2worldY.y().getVector3().crossProduct( 
      											 		_local2worldY.z().getVector3()).getNormalised();
      matrix4_premultiply_by_matrix4(_local2worldY, _pivot._worldSpace);
    }

    _circleZ_visible = !vector3_equal_epsilon(g_vector3_axis_z, localViewpoint, 1e-6);
    if(_circleZ_visible)
    {
      _local2worldZ = Matrix4::getIdentity();
      _local2worldZ.x().getVector3() = g_vector3_axis_z.crossProduct(localViewpoint).getNormalised();
      _local2worldZ.y().getVector3() = _local2worldZ.z().getVector3().crossProduct( 
      												_local2worldZ.x().getVector3()).getNormalised();
      matrix4_premultiply_by_matrix4(_local2worldZ, _pivot._worldSpace);
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

void RotateManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());
    updateCircleTransforms();

    SelectionPool selector;

    {
      {
        Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _local2worldX));

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, &_circleX.front(), _circleX.size(), best);
        selector.addSelectable(best, &_selectableX);
      }

      {
        Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _local2worldY));

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, &_circleY.front(), _circleY.size(), best);
        selector.addSelectable(best, &_selectableY);
      }

      {
        Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _local2worldZ));

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, &_circleZ.front(), _circleZ.size(), best);
        selector.addSelectable(best, &_selectableZ);
      }
    }

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _pivot._viewpointSpace));

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

Manipulatable* RotateManipulator::GetManipulatable() {
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


// ------------ TranslateManipulator methods ------------------

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
    _quadScreen.setColour(colourSelected(g_colour_screen, _selectableScreen.isSelected()));
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
  
void TranslateManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    Vector3 x = _pivot._worldSpace.x().getVector3().getNormalised();
    bool show_x = manipulator_show_axis(_pivot, x);

    Vector3 y = _pivot._worldSpace.y().getVector3().getNormalised();
    bool show_y = manipulator_show_axis(_pivot, y);

    Vector3 z = _pivot._worldSpace.z().getVector3().getNormalised();
    bool show_z = manipulator_show_axis(_pivot, z);

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _pivot._viewpointSpace));

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
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _pivot._worldSpace));

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
    	
    	if (GlobalRegistry().get(RKEY_TRANSLATE_CONSTRAINED) == "1") {
	    	// None of the shown arrows (or quad) has been selected, select an axis based on the precedence
	    	Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _pivot._worldSpace));
	
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

Manipulatable* TranslateManipulator::GetManipulatable() {
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


// ------------ ScaleManipulator methods ------------------

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
    _quadScreen.setColour(colourSelected(g_colour_screen, _selectableScreen.isSelected()));
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

void ScaleManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    _pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _pivot._worldSpace));

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
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), _pivot._viewpointSpace));

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

Manipulatable* ScaleManipulator::GetManipulatable() {
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

// ------------ DragManipulator methods ------------------

Manipulatable* DragManipulator::GetManipulatable() {
    return _dragSelectable.isSelected() ? &_freeDrag : &_freeResize;
}

void DragManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    SelectionPool selector;

    SelectionVolume test(view);

    if(GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive)
    {
    	// Find all entities
		BooleanSelector entitySelector;

		testselect_entity_visible selectionTest(entitySelector, test);

		Scene_forEachVisible(GlobalSceneGraph(), view, selectionTest);
    	
    	// Find all primitives that are selectable 
		BooleanSelector booleanSelector;
		Scene_TestSelect_Primitive(booleanSelector, test, view, true);

		if (entitySelector.isSelected()) {
			// Found a selectable entity
			selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
			_selected = false;
		} else if(booleanSelector.isSelected())	{
			// Found a selectable primitive
			selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
			_selected = false;
		}
		else {
			// Check for selectable faces
			_selected = Scene_forEachPlaneSelectable_selectPlanes(selector, test);
		}
    }
    // Check for entities that can be selected
    else if(GlobalSelectionSystem().Mode() == SelectionSystem::eEntity) {
    	// Create a boolean selection pool (can have exactly one selectable or none)
		BooleanSelector booleanSelector;
	
		// Find the visible entities
		testselect_entity_visible tester(booleanSelector, test);
		Scene_forEachVisible(GlobalSceneGraph(), view, tester);

		// Check, if an entity could be found
      	if (booleanSelector.isSelected()) {
        	selector.addSelectable(SelectionIntersection(0, 0), &_dragSelectable);
        	_selected = false;
		}
    }
    else
    {
      BestSelector bestSelector;
      Scene_TestSelect_Component_Selected(bestSelector, test, view, GlobalSelectionSystem().ComponentMode());
      for(std::list<Selectable*>::iterator i = bestSelector.best().begin(); i != bestSelector.best().end(); ++i)
      {
      	/** greebo: Disabled this, it caused the currently selected patch vertices being deselected.
      	 */
      	if (GlobalRegistry().get(RKEY_TRANSIENT_COMPONENT_SELECTION) == "1") {
      		if(!(*i)->isSelected()) {
          		GlobalSelectionSystem().setSelectedAllComponents(false);
      		}
      	}
        _selected = false;
        selector.addSelectable(SelectionIntersection(0, 0), (*i));
        _dragSelectable.setSelected(true);
      }
    }

	for(SelectionPool::iterator i = selector.begin(); i != selector.end(); ++i) {
		i->second->setSelected(true);
	}
}

void DragManipulator::setSelected(bool select) {
    _selected = select;
    _dragSelectable.setSelected(select);
}

bool DragManipulator::isSelected() const  {
	return _selected || _dragSelectable.isSelected();
}
