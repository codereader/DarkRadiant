#ifndef CAMRENDERER_H_
#define CAMRENDERER_H_

#include "renderer.h"

class CamRenderer :
	public RenderableCollector
{
private:

	struct state_type
	{
		state_type() : 
			highlight(0), 
			state(NULL), 
			lights(0)
		{}

		unsigned int highlight;

		// use raw pointers, this gets assigned very, very often
		Shader* state;	

		const LightList* lights;
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

	// avoid reallocations
	m_state_stack.reserve(10);

    m_state_stack.push_back(state_type());
  }

  void SetState(const ShaderPtr& state, EStyle style)
  {
    if(style == eFullMaterials)
    {
		ASSERT_NOTNULL(state);
		m_state_stack.back().state = state.get();
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
      ? m_state_stack.back().highlight |= mode
      : m_state_stack.back().highlight &= ~mode;
  }
  void setLights(const LightList& lights)
  {
    m_state_stack.back().lights = &lights;
  }
  void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world)
  {
    if(m_state_stack.back().highlight & ePrimitive)
    {
      m_state_select0->addRenderable(renderable, world, m_state_stack.back().lights);
    }
    if(m_state_stack.back().highlight & eFace)
    {
      m_state_select1->addRenderable(renderable, world, m_state_stack.back().lights);
    }

    m_state_stack.back().state->addRenderable(renderable, world, m_state_stack.back().lights);
  }

  void render(const Matrix4& modelview, const Matrix4& projection)
  {
    GlobalRenderSystem().render(m_globalstate, modelview, projection, m_viewer);
  }
}; // class CamRenderer


#endif /*CAMRENDERER_H_*/
