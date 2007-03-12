#ifndef XYRENDERER_H_
#define XYRENDERER_H_

#include "renderable.h"

class XYRenderer : 
	public Renderer
{
	struct state_type {
		state_type() 
		: m_highlight(0) {}
		
		unsigned int m_highlight;
		ShaderPtr m_state;
	};
	
	std::vector<state_type> m_state_stack;
	RenderStateFlags m_globalstate;
	ShaderPtr m_state_selected;

public:
	XYRenderer(RenderStateFlags globalstate, ShaderPtr selected) :
			m_globalstate(globalstate),
			m_state_selected(selected) {
		ASSERT_NOTNULL(selected);
		m_state_stack.push_back(state_type());
	}

	void SetState(ShaderPtr state, EStyle style) {
		ASSERT_NOTNULL(state);
		if (style == eWireframeOnly)
			m_state_stack.back().m_state = state;
	}
	
	const EStyle getStyle() const {
		return eWireframeOnly;
	}
	
	void PushState() {
		m_state_stack.push_back(m_state_stack.back());
	}
	
	void PopState() {
		ASSERT_MESSAGE(!m_state_stack.empty(), "popping empty stack");
		m_state_stack.pop_back();
	}
	
	void Highlight(EHighlightMode mode, bool bEnable = true) {
		(bEnable)
		? m_state_stack.back().m_highlight |= mode
		                                      : m_state_stack.back().m_highlight &= ~mode;
	}
	
	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& localToWorld) {
		if (m_state_stack.back().m_highlight & ePrimitive) {
			m_state_selected->addRenderable(renderable, localToWorld);
		}
		else {
			m_state_stack.back().m_state->addRenderable(renderable, localToWorld);
		}
	}

	void render(const Matrix4& modelview, const Matrix4& projection) {
		GlobalShaderCache().render(m_globalstate, modelview, projection);
	}
}; // class XYRenderer

#endif /*XYRENDERER_H_*/
