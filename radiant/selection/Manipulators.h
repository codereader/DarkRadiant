#ifndef MANIPULATORS_H_
#define MANIPULATORS_H_

/* greebo: This file contains the manipulator classes like 
 * 
 * - Translate Maniplator
 * - Rotate Manipulator
 * - Scale Manipulator
 * 
 * that derive from the abstract base class Manipulator
 */

#include "renderable.h"
#include "pivot.h"
#include "view.h"
#include "math/matrix.h"
#include "selectionlib.h"
#include "Manipulatables.h"

struct Pivot2World
{
  Matrix4 m_worldSpace;
  Matrix4 m_viewpointSpace;
  Matrix4 m_viewplaneSpace;
  Vector3 m_axis_screen;

  void update(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
  {
    Pivot2World_worldSpace(m_worldSpace, pivot2world, modelview, projection, viewport);
    Pivot2World_viewpointSpace(m_viewpointSpace, m_axis_screen, pivot2world, modelview, projection, viewport);
    Pivot2World_viewplaneSpace(m_viewplaneSpace, pivot2world, modelview, projection, viewport);
  }
};

// The abstract base class for a manipulator
class Manipulator
{
public:
  virtual Manipulatable* GetManipulatable() = 0;
  virtual void testSelect(const View& view, const Matrix4& pivot2world)
  {
  }
  // This function is responsible for bringing the visual representation
  // of this manipulator onto the screen
  virtual void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world)
  {
  }
  virtual void setSelected(bool select) = 0;
  virtual bool isSelected() const = 0;
};

// =======================================================================================

/*	The Manipulator for Rotations
 */
class RotateManipulator : public Manipulator
{

  // helper class for rendering a circle
  struct RenderableCircle : public OpenGLRenderable
  {
    Array<PointVertex> m_vertices;

    RenderableCircle(std::size_t size) : m_vertices(size)
    {
    }
    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_vertices.data()->colour);
      glVertexPointer(3, GL_FLOAT, sizeof(PointVertex), &m_vertices.data()->vertex);
      glDrawArrays(GL_LINE_LOOP, 0, GLsizei(m_vertices.size()));
    }
    void setColour(const Colour4b& colour)
    {
      for(Array<PointVertex>::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
      {
        (*i).colour = colour;
      }
    }
  };

  // helper class for rendering a semi-circle
  struct RenderableSemiCircle : public OpenGLRenderable
  {
    Array<PointVertex> m_vertices;

    RenderableSemiCircle(std::size_t size) : m_vertices(size)
    {
    }
    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_vertices.data()->colour);
      glVertexPointer(3, GL_FLOAT, sizeof(PointVertex), &m_vertices.data()->vertex);
      glDrawArrays(GL_LINE_STRIP, 0, GLsizei(m_vertices.size()));
    }
    void setColour(const Colour4b& colour)
    {
      for(Array<PointVertex>::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
      {
        (*i).colour = colour;
      }
    }
  };

  RotateFree m_free;
  RotateAxis m_axis;
  Vector3 m_axis_screen;
  RenderableSemiCircle m_circle_x;
  RenderableSemiCircle m_circle_y;
  RenderableSemiCircle m_circle_z;
  RenderableCircle m_circle_screen;
  RenderableCircle m_circle_sphere;
  SelectableBool m_selectable_x;
  SelectableBool m_selectable_y;
  SelectableBool m_selectable_z;
  SelectableBool m_selectable_screen;
  SelectableBool m_selectable_sphere;
  Pivot2World m_pivot;
  Matrix4 m_local2world_x;
  Matrix4 m_local2world_y;
  Matrix4 m_local2world_z;
  bool m_circle_x_visible;
  bool m_circle_y_visible;
  bool m_circle_z_visible;
  
public:
  static Shader* m_state_outer;

  // Constructor
  RotateManipulator(Rotatable& rotatable, std::size_t segments, float radius);

  void UpdateColours();
  void updateCircleTransforms();
  
  void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world);
  
  void testSelect(const View& view, const Matrix4& pivot2world);
  
  Manipulatable* GetManipulatable();
  
  void setSelected(bool select);  
  bool isSelected() const;  
};

// =======================================================================================

/* The Manipulator for translation operations
 */ 
class TranslateManipulator : public Manipulator
{
	// Helper class
  struct RenderableArrowLine : public OpenGLRenderable
  {
    PointVertex m_line[2];

    RenderableArrowLine()
    {
    }
    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_line[0].colour);
      glVertexPointer(3, GL_FLOAT, sizeof(PointVertex), &m_line[0].vertex);
      glDrawArrays(GL_LINES, 0, 2);
    }
    void setColour(const Colour4b& colour)
    {
      m_line[0].colour = colour;
      m_line[1].colour = colour;
    }
  };
  
  // Helper class
  struct RenderableArrowHead : public OpenGLRenderable
  {
    Array<FlatShadedVertex> m_vertices;

    RenderableArrowHead(std::size_t size)
      : m_vertices(size)
    {
    }
    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(FlatShadedVertex), &m_vertices.data()->colour);
      glVertexPointer(3, GL_FLOAT, sizeof(FlatShadedVertex), &m_vertices.data()->vertex);
      glNormalPointer(GL_FLOAT, sizeof(FlatShadedVertex), &m_vertices.data()->normal);
      glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_vertices.size()));
    }
    void setColour(const Colour4b& colour)
    {
      for(Array<FlatShadedVertex>::iterator i = m_vertices.begin(); i != m_vertices.end(); ++i)
      {
        (*i).colour = colour;
      }
    }
  };
  
  // Helper class
  struct RenderableQuad : public OpenGLRenderable
  {
    PointVertex m_quad[4];
    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_quad[0].colour);
      glVertexPointer(3, GL_FLOAT, sizeof(PointVertex), &m_quad[0].vertex);
      glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    void setColour(const Colour4b& colour)
    {
      m_quad[0].colour = colour;
      m_quad[1].colour = colour;
      m_quad[2].colour = colour;
      m_quad[3].colour = colour;
    }
  };

  TranslateFree m_free;
  TranslateAxis m_axis;
  RenderableArrowLine m_arrow_x;
  RenderableArrowLine m_arrow_y;
  RenderableArrowLine m_arrow_z;
  RenderableArrowHead m_arrow_head_x;
  RenderableArrowHead m_arrow_head_y;
  RenderableArrowHead m_arrow_head_z;
  RenderableQuad m_quad_screen;
  SelectableBool m_selectable_x;
  SelectableBool m_selectable_y;
  SelectableBool m_selectable_z;
  SelectableBool m_selectable_screen;
  Pivot2World m_pivot;
public:
  static Shader* m_state_wire;
  static Shader* m_state_fill;

  // Constructor
  TranslateManipulator(Translatable& translatable, std::size_t segments, float length);

  void UpdateColours();
  bool manipulator_show_axis(const Pivot2World& pivot, const Vector3& axis);
  
  void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world);
  void testSelect(const View& view, const Matrix4& pivot2world);
  Manipulatable* GetManipulatable();
  
  void setSelected(bool select);
  bool isSelected() const;  
}; // class TranslateManipulator

// =======================================================================================

/* The Manipulator for scale operations
 */ 
class ScaleManipulator : public Manipulator
{
	// Helper class
  struct RenderableArrow : public OpenGLRenderable
  {
    PointVertex m_line[2];

    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_line[0].colour);
      glVertexPointer(3, GL_FLOAT, sizeof(PointVertex), &m_line[0].vertex);
      glDrawArrays(GL_LINES, 0, 2);
    }
    void setColour(const Colour4b& colour)
    {
      m_line[0].colour = colour;
      m_line[1].colour = colour;
    }
  };
  
  // Helper class
  struct RenderableQuad : public OpenGLRenderable
  {
    PointVertex m_quad[4];
    void render(RenderStateFlags state) const
    {
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(PointVertex), &m_quad[0].colour);
      glVertexPointer(3, GL_FLOAT, sizeof(PointVertex), &m_quad[0].vertex);
      glDrawArrays(GL_QUADS, 0, 4);
    }
    void setColour(const Colour4b& colour)
    {
      m_quad[0].colour = colour;
      m_quad[1].colour = colour;
      m_quad[2].colour = colour;
      m_quad[3].colour = colour;
    }
  };

  ScaleFree m_free;
  ScaleAxis m_axis;
  RenderableArrow m_arrow_x;
  RenderableArrow m_arrow_y;
  RenderableArrow m_arrow_z;
  RenderableQuad m_quad_screen;
  SelectableBool m_selectable_x;
  SelectableBool m_selectable_y;
  SelectableBool m_selectable_z;
  SelectableBool m_selectable_screen;
  Pivot2World m_pivot;
  
public:

  // Constructor
  ScaleManipulator(Scalable& scalable, std::size_t segments, float length);

  Pivot2World& getPivot() {
    return m_pivot;
  }

  void UpdateColours();
  
  void render(Renderer& renderer, const VolumeTest& volume, const Matrix4& pivot2world);
  void testSelect(const View& view, const Matrix4& pivot2world);
  Manipulatable* GetManipulatable();
  
  void setSelected(bool select);
  bool isSelected() const;
  
}; // class ScaleManipulator

#endif /*MANIPULATORS_H_*/
