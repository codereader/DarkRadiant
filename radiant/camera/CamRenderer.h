#ifndef CAMRENDERER_H_
#define CAMRENDERER_H_

#include "renderer.h"

class CamRenderer: public Renderer {
  struct state_type
  {
    state_type() : m_highlight(0), m_lights(0)
    {
    }  
    unsigned int m_highlight;
    ShaderPtr m_state;
    const LightList* m_lights;
  };

  std::vector<state_type> m_state_stack;
  RenderStateFlags m_globalstate;
  ShaderPtr m_state_select0;
  ShaderPtr m_state_select1;
  const Vector3& m_viewer;

public:
  CamRenderer(RenderStateFlags globalstate, ShaderPtr select0, ShaderPtr select1, const Vector3& viewer) :
    m_globalstate(globalstate),
    m_state_select0(select0),
    m_state_select1(select1),
    m_viewer(viewer)
  {
    ASSERT_NOTNULL(select0);
    ASSERT_NOTNULL(select1);
    m_state_stack.push_back(state_type());
  }

  void SetState(ShaderPtr state, EStyle style)
  {
    ASSERT_NOTNULL(state);
    if(style == eFullMaterials)
    {
      m_state_stack.back().m_state = state;
    }
  }
  const EStyle getStyle() const
  {
    return eFullMaterials;
  }
  void PushState()
  {
    m_state_stack.push_back(m_state_stack.back());
  }
  void PopState()
  {
    ASSERT_MESSAGE(!m_state_stack.empty(), "popping empty stack");
    m_state_stack.pop_back();
  }
  void Highlight(EHighlightMode mode, bool bEnable = true)
  {
    (bEnable)
      ? m_state_stack.back().m_highlight |= mode
      : m_state_stack.back().m_highlight &= ~mode;
  }
  void setLights(const LightList& lights)
  {
    m_state_stack.back().m_lights = &lights;
  }
  void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world)
  {
    if(m_state_stack.back().m_highlight & ePrimitive)
    {
      m_state_select0->addRenderable(renderable, world, m_state_stack.back().m_lights);
    }
    if(m_state_stack.back().m_highlight & eFace)
    {
      m_state_select1->addRenderable(renderable, world, m_state_stack.back().m_lights);
    }

    m_state_stack.back().m_state->addRenderable(renderable, world, m_state_stack.back().m_lights);
  }

  void render(const Matrix4& modelview, const Matrix4& projection)
  {
    GlobalShaderCache().render(m_globalstate, modelview, projection, m_viewer);
  }
}; // class CamRenderer


#endif /*CAMRENDERER_H_*/
