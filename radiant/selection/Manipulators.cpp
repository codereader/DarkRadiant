#include "Manipulators.h"
#include "Remap.h"
#include "Selector.h"
#include "BestPoint.h"

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
    m_free(rotatable),
    m_axis(rotatable),
    m_circle_x((segments << 2) + 1),
    m_circle_y((segments << 2) + 1),
    m_circle_z((segments << 2) + 1),
    m_circle_screen(segments<<3),
    m_circle_sphere(segments<<3)
{
	draw_semicircle(segments, radius, m_circle_x.m_vertices.data(), RemapYZX());
    draw_semicircle(segments, radius, m_circle_y.m_vertices.data(), RemapZXY());
    draw_semicircle(segments, radius, m_circle_z.m_vertices.data(), RemapXYZ());

    draw_circle(segments, radius * 1.15f, m_circle_screen.m_vertices.data(), RemapXYZ());
    draw_circle(segments, radius, m_circle_sphere.m_vertices.data(), RemapXYZ());
}

void RotateManipulator::UpdateColours()  {
    m_circle_x.setColour(colourSelected(g_colour_x, m_selectable_x.isSelected()));
    m_circle_y.setColour(colourSelected(g_colour_y, m_selectable_y.isSelected()));
    m_circle_z.setColour(colourSelected(g_colour_z, m_selectable_z.isSelected()));
    m_circle_screen.setColour(colourSelected(g_colour_screen, m_selectable_screen.isSelected()));
    m_circle_sphere.setColour(colourSelected(g_colour_sphere, false));
}
  
void RotateManipulator::updateCircleTransforms()  {
    Vector3 localViewpoint(matrix4_transformed_direction(matrix4_transposed(m_pivot.m_worldSpace), vector4_to_vector3(m_pivot.m_viewpointSpace.z())));

    m_circle_x_visible = !vector3_equal_epsilon(g_vector3_axis_x, localViewpoint, 1e-6f);
    if(m_circle_x_visible)
    {
      m_local2world_x = g_matrix4_identity;
      vector4_to_vector3(m_local2world_x.y()) = g_vector3_axis_x.crossProduct(localViewpoint).getNormalised();
      vector4_to_vector3(m_local2world_x.z()) = vector4_to_vector3(m_local2world_x.x()).crossProduct( 
        											vector4_to_vector3(m_local2world_x.y())).getNormalised();
      matrix4_premultiply_by_matrix4(m_local2world_x, m_pivot.m_worldSpace);
    }

    m_circle_y_visible = !vector3_equal_epsilon(g_vector3_axis_y, localViewpoint, 1e-6f);
    if(m_circle_y_visible)
    {
      m_local2world_y = g_matrix4_identity;
      vector4_to_vector3(m_local2world_y.z()) = g_vector3_axis_y.crossProduct(localViewpoint).getNormalised();
      vector4_to_vector3(m_local2world_y.x()) = vector4_to_vector3(m_local2world_y.y()).crossProduct( 
      											 		vector4_to_vector3(m_local2world_y.z())).getNormalised();
      matrix4_premultiply_by_matrix4(m_local2world_y, m_pivot.m_worldSpace);
    }

    m_circle_z_visible = !vector3_equal_epsilon(g_vector3_axis_z, localViewpoint, 1e-6f);
    if(m_circle_z_visible)
    {
      m_local2world_z = g_matrix4_identity;
      vector4_to_vector3(m_local2world_z.x()) = g_vector3_axis_z.crossProduct(localViewpoint).getNormalised();
      vector4_to_vector3(m_local2world_z.y()) = vector4_to_vector3(m_local2world_z.z()).crossProduct( 
      												vector4_to_vector3(m_local2world_z.x())).getNormalised();
      matrix4_premultiply_by_matrix4(m_local2world_z, m_pivot.m_worldSpace);
    }
}

void RotateManipulator::render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world) {
    m_pivot.update(pivot2world, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());
    updateCircleTransforms();

    // temp hack
    UpdateColours();

    renderer.SetState(m_state_outer, Renderer::eWireframeOnly);
    renderer.SetState(m_state_outer, Renderer::eFullMaterials);

    renderer.addRenderable(m_circle_screen, m_pivot.m_viewpointSpace);
    renderer.addRenderable(m_circle_sphere, m_pivot.m_viewpointSpace);

    if(m_circle_x_visible)
    {
      renderer.addRenderable(m_circle_x, m_local2world_x);
    }
    if(m_circle_y_visible)
    {
      renderer.addRenderable(m_circle_y, m_local2world_y);
    }
    if(m_circle_z_visible)
    {
      renderer.addRenderable(m_circle_z, m_local2world_z);
    }
}

void RotateManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    m_pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());
    updateCircleTransforms();

    SelectionPool selector;

    {
      {
        Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_local2world_x));

#if defined(DEBUG_SELECTION)
        g_render_clipped.construct(view.GetViewMatrix());
#endif

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, m_circle_x.m_vertices.data(), m_circle_x.m_vertices.size(), best);
        selector.addSelectable(best, &m_selectable_x);
      }

      {
        Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_local2world_y));

#if defined(DEBUG_SELECTION)
        g_render_clipped.construct(view.GetViewMatrix());
#endif

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, m_circle_y.m_vertices.data(), m_circle_y.m_vertices.size(), best);
        selector.addSelectable(best, &m_selectable_y);
      }

      {
        Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_local2world_z));

#if defined(DEBUG_SELECTION)
        g_render_clipped.construct(view.GetViewMatrix());
#endif

        SelectionIntersection best;
        LineStrip_BestPoint(local2view, m_circle_z.m_vertices.data(), m_circle_z.m_vertices.size(), best);
        selector.addSelectable(best, &m_selectable_z);
      }
    }

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_pivot.m_viewpointSpace));

      {
        SelectionIntersection best;
        LineLoop_BestPoint(local2view, m_circle_screen.m_vertices.data(), m_circle_screen.m_vertices.size(), best);
        selector.addSelectable(best, &m_selectable_screen); 
      }

      {
        SelectionIntersection best;
        Circle_BestPoint(local2view, eClipCullCW, m_circle_sphere.m_vertices.data(), m_circle_sphere.m_vertices.size(), best);
        selector.addSelectable(best, &m_selectable_sphere); 
      }
    }

    m_axis_screen = m_pivot.m_axis_screen;

    if(!selector.failed())
    {
      (*selector.begin()).second->setSelected(true);
    }
}

Manipulatable* RotateManipulator::GetManipulatable() {
    if(m_selectable_x.isSelected()) {
      m_axis.SetAxis(g_vector3_axis_x);
      return &m_axis;
    }
    else if(m_selectable_y.isSelected()) {
      m_axis.SetAxis(g_vector3_axis_y);
      return &m_axis;
    }
    else if(m_selectable_z.isSelected()) {
      m_axis.SetAxis(g_vector3_axis_z);
      return &m_axis;
    }
    else if(m_selectable_screen.isSelected()) {
      m_axis.SetAxis(m_axis_screen);
      return &m_axis;
    }
    else
      return &m_free;
}

void RotateManipulator::setSelected(bool select)  {
    m_selectable_x.setSelected(select);
    m_selectable_y.setSelected(select);
    m_selectable_z.setSelected(select);
    m_selectable_screen.setSelected(select);
}

bool RotateManipulator::isSelected() const {
    return m_selectable_x.isSelected()
      | m_selectable_y.isSelected()
      | m_selectable_z.isSelected()
      | m_selectable_screen.isSelected()
      | m_selectable_sphere.isSelected();
}

// Initialise the shader of the RotateManipulator class
Shader* RotateManipulator::m_state_outer;


// ------------ TranslateManipulator methods ------------------

// Constructor
TranslateManipulator::TranslateManipulator(Translatable& translatable, std::size_t segments, float length) :
    m_free(translatable),
    m_axis(translatable),
    m_arrow_head_x(3 * 2 * (segments << 3)),
    m_arrow_head_y(3 * 2 * (segments << 3)),
    m_arrow_head_z(3 * 2 * (segments << 3))
{
    draw_arrowline(length, m_arrow_x.m_line, 0);
    draw_arrowhead(segments, length, m_arrow_head_x.m_vertices.data(), TripleRemapXYZ<Vertex3f>(), TripleRemapXYZ<Normal3f>());
    draw_arrowline(length, m_arrow_y.m_line, 1);
    draw_arrowhead(segments, length, m_arrow_head_y.m_vertices.data(), TripleRemapYZX<Vertex3f>(), TripleRemapYZX<Normal3f>());
    draw_arrowline(length, m_arrow_z.m_line, 2);
    draw_arrowhead(segments, length, m_arrow_head_z.m_vertices.data(), TripleRemapZXY<Vertex3f>(), TripleRemapZXY<Normal3f>());

    draw_quad(16, m_quad_screen.m_quad);
}

void TranslateManipulator::UpdateColours() {
    m_arrow_x.setColour(colourSelected(g_colour_x, m_selectable_x.isSelected()));
    m_arrow_head_x.setColour(colourSelected(g_colour_x, m_selectable_x.isSelected()));
    m_arrow_y.setColour(colourSelected(g_colour_y, m_selectable_y.isSelected()));
    m_arrow_head_y.setColour(colourSelected(g_colour_y, m_selectable_y.isSelected()));
    m_arrow_z.setColour(colourSelected(g_colour_z, m_selectable_z.isSelected()));
    m_arrow_head_z.setColour(colourSelected(g_colour_z, m_selectable_z.isSelected()));
    m_quad_screen.setColour(colourSelected(g_colour_screen, m_selectable_screen.isSelected()));
}

bool TranslateManipulator::manipulator_show_axis(const Pivot2World& pivot, const Vector3& axis) {
    return fabs(pivot.m_axis_screen.dot(axis)) < 0.95;
}

void TranslateManipulator::render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world) {
    m_pivot.update(pivot2world, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    // temp hack
    UpdateColours();

    Vector3 x = vector4_to_vector3(m_pivot.m_worldSpace.x()).getNormalised();
    bool show_x = manipulator_show_axis(m_pivot, x);

    Vector3 y = vector4_to_vector3(m_pivot.m_worldSpace.y()).getNormalised();
    bool show_y = manipulator_show_axis(m_pivot, y);

    Vector3 z = vector4_to_vector3(m_pivot.m_worldSpace.z()).getNormalised();
    bool show_z = manipulator_show_axis(m_pivot, z);

    renderer.SetState(m_state_wire, Renderer::eWireframeOnly);
    renderer.SetState(m_state_wire, Renderer::eFullMaterials);

    if(show_x)
    {
      renderer.addRenderable(m_arrow_x, m_pivot.m_worldSpace);
    }
    if(show_y)
    {
      renderer.addRenderable(m_arrow_y, m_pivot.m_worldSpace);
    }
    if(show_z)
    {
      renderer.addRenderable(m_arrow_z, m_pivot.m_worldSpace);
    }

    renderer.addRenderable(m_quad_screen, m_pivot.m_viewplaneSpace);

    renderer.SetState(m_state_fill, Renderer::eWireframeOnly);
    renderer.SetState(m_state_fill, Renderer::eFullMaterials);

    if(show_x)
    {
      renderer.addRenderable(m_arrow_head_x, m_pivot.m_worldSpace);
    }
    if(show_y)
    {
      renderer.addRenderable(m_arrow_head_y, m_pivot.m_worldSpace);
    }
    if(show_z)
    {
      renderer.addRenderable(m_arrow_head_z, m_pivot.m_worldSpace);
    }
}
  
void TranslateManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    m_pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    Vector3 x = vector4_to_vector3(m_pivot.m_worldSpace.x()).getNormalised();
    bool show_x = manipulator_show_axis(m_pivot, x);

    Vector3 y = vector4_to_vector3(m_pivot.m_worldSpace.y()).getNormalised();
    bool show_y = manipulator_show_axis(m_pivot, y);

    Vector3 z = vector4_to_vector3(m_pivot.m_worldSpace.z()).getNormalised();
    bool show_z = manipulator_show_axis(m_pivot, z);

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_pivot.m_viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, m_quad_screen.m_quad, best);
        if(best.valid())
        {
          best = SelectionIntersection(0, 0);
          selector.addSelectable(best, &m_selectable_screen);
        }
      }
    }

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_pivot.m_worldSpace));

#if defined(DEBUG_SELECTION)
      g_render_clipped.construct(view.GetViewMatrix());
#endif

      if(show_x)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, m_arrow_x.m_line, best);
        Triangles_BestPoint(local2view, eClipCullCW, m_arrow_head_x.m_vertices.begin(), m_arrow_head_x.m_vertices.end(), best);
        selector.addSelectable(best, &m_selectable_x);
      }

      if(show_y)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, m_arrow_y.m_line, best);
        Triangles_BestPoint(local2view, eClipCullCW, m_arrow_head_y.m_vertices.begin(), m_arrow_head_y.m_vertices.end(), best);
        selector.addSelectable(best, &m_selectable_y);
      }

      if(show_z)
      {
        SelectionIntersection best;
        Line_BestPoint(local2view, m_arrow_z.m_line, best);
        Triangles_BestPoint(local2view, eClipCullCW, m_arrow_head_z.m_vertices.begin(), m_arrow_head_z.m_vertices.end(), best);
        selector.addSelectable(best, &m_selectable_z);
      }
    }

    if(!selector.failed())
    {
      (*selector.begin()).second->setSelected(true);
    }
}

Manipulatable* TranslateManipulator::GetManipulatable() {
    if(m_selectable_x.isSelected())
    {
      m_axis.SetAxis(g_vector3_axis_x);
      return &m_axis;
    }
    else if(m_selectable_y.isSelected())
    {
      m_axis.SetAxis(g_vector3_axis_y);
      return &m_axis;
    }
    else if(m_selectable_z.isSelected())
    {
      m_axis.SetAxis(g_vector3_axis_z);
      return &m_axis;
    }
    else
    {
      return &m_free;
    }
}

void TranslateManipulator::setSelected(bool select) {
    m_selectable_x.setSelected(select);
    m_selectable_y.setSelected(select);
    m_selectable_z.setSelected(select);
    m_selectable_screen.setSelected(select);
}

bool TranslateManipulator::isSelected() const {
    return m_selectable_x.isSelected()
      | m_selectable_y.isSelected()
      | m_selectable_z.isSelected()
      | m_selectable_screen.isSelected();
}

// Initialise the shaders of this class
Shader* TranslateManipulator::m_state_wire;
Shader* TranslateManipulator::m_state_fill;


// ------------ ScaleManipulator methods ------------------

// Constructor
ScaleManipulator::ScaleManipulator(Scalable& scalable, std::size_t segments, float length) :
    m_free(scalable),
    m_axis(scalable)
{
    draw_arrowline(length, m_arrow_x.m_line, 0);
    draw_arrowline(length, m_arrow_y.m_line, 1);
    draw_arrowline(length, m_arrow_z.m_line, 2);

    draw_quad(16, m_quad_screen.m_quad);
}

void ScaleManipulator::UpdateColours() {
    m_arrow_x.setColour(colourSelected(g_colour_x, m_selectable_x.isSelected()));
    m_arrow_y.setColour(colourSelected(g_colour_y, m_selectable_y.isSelected()));
    m_arrow_z.setColour(colourSelected(g_colour_z, m_selectable_z.isSelected()));
    m_quad_screen.setColour(colourSelected(g_colour_screen, m_selectable_screen.isSelected()));
}

void ScaleManipulator::render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world) {
    m_pivot.update(pivot2world, volume.GetModelview(), volume.GetProjection(), volume.GetViewport());

    // temp hack
    UpdateColours();

    renderer.addRenderable(m_arrow_x, m_pivot.m_worldSpace);
    renderer.addRenderable(m_arrow_y, m_pivot.m_worldSpace);
    renderer.addRenderable(m_arrow_z, m_pivot.m_worldSpace);

    renderer.addRenderable(m_quad_screen, m_pivot.m_viewpointSpace);
}

void ScaleManipulator::testSelect(const View& view, const Matrix4& pivot2world) {
    m_pivot.update(pivot2world, view.GetModelview(), view.GetProjection(), view.GetViewport());

    SelectionPool selector;

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_pivot.m_worldSpace));

#if defined(DEBUG_SELECTION)
      g_render_clipped.construct(view.GetViewMatrix());
#endif

    {
        SelectionIntersection best;
        Line_BestPoint(local2view, m_arrow_x.m_line, best);
        selector.addSelectable(best, &m_selectable_x);
      }

      {
        SelectionIntersection best;
        Line_BestPoint(local2view, m_arrow_y.m_line, best);
        selector.addSelectable(best, &m_selectable_y);
      }

      {
        SelectionIntersection best;
        Line_BestPoint(local2view, m_arrow_z.m_line, best);
        selector.addSelectable(best, &m_selectable_z);
      }
    }

    {
      Matrix4 local2view(matrix4_multiplied_by_matrix4(view.GetViewMatrix(), m_pivot.m_viewpointSpace));

      {
        SelectionIntersection best;
        Quad_BestPoint(local2view, eClipCullCW, m_quad_screen.m_quad, best);
        selector.addSelectable(best, &m_selectable_screen);
      }
    }

    if(!selector.failed())
    {
      (*selector.begin()).second->setSelected(true);
    }
}

Manipulatable* ScaleManipulator::GetManipulatable() {
    if(m_selectable_x.isSelected())
    {
      m_axis.SetAxis(g_vector3_axis_x);
      return &m_axis;
    }
    else if(m_selectable_y.isSelected())
    {
      m_axis.SetAxis(g_vector3_axis_y);
      return &m_axis;
    }
    else if(m_selectable_z.isSelected())
    {
      m_axis.SetAxis(g_vector3_axis_z);
      return &m_axis;
    }
    else
      return &m_free;
}

void ScaleManipulator::setSelected(bool select) {
    m_selectable_x.setSelected(select);
    m_selectable_y.setSelected(select);
    m_selectable_z.setSelected(select);
    m_selectable_screen.setSelected(select);
}

bool ScaleManipulator::isSelected() const {
    return m_selectable_x.isSelected()
      | m_selectable_y.isSelected()
      | m_selectable_z.isSelected()
      | m_selectable_screen.isSelected();
}
