#ifndef XYRENDERER_H_
#define XYRENDERER_H_

#include "irenderable.h"

class XYRenderer : 
	public Renderer
{
	// State type structure
	struct state_type {
		
		unsigned int _highlight;
		
		// The actual shader. This is a raw pointer for performance, since we
		// know that the Shader will exist for the lifetime of this render
		// operation.
		Shader* _state;

		// Constructor
		state_type() 
		: _highlight(0), _state(NULL) 
		{}
		
	};
	
	std::vector<state_type> m_state_stack;
	RenderStateFlags m_globalstate;
	
	// Shader to use for highlighted objects
	Shader* _selectedShader;

public:
	XYRenderer(RenderStateFlags globalstate, Shader* selected) :
			m_globalstate(globalstate),
			_selectedShader(selected) 
	{
		// Reserve space in the vector to avoid reallocation delays
		m_state_stack.reserve(8);
		
		m_state_stack.push_back(state_type());
	}

	void SetState(ShaderPtr state, EStyle style) {
		ASSERT_NOTNULL(state);
		if (style == eWireframeOnly)
			m_state_stack.back()._state = state.get();
	}
	
	const EStyle getStyle() const {
		return eWireframeOnly;
	}
	
	void PushState() {
		// Duplicate the most recent state
		m_state_stack.push_back(m_state_stack.back());
	}
	
	void PopState() {
		m_state_stack.pop_back();
	}
	
	void Highlight(EHighlightMode mode, bool bEnable = true) {
		(bEnable) ? m_state_stack.back()._highlight |= mode
		          : m_state_stack.back()._highlight &= ~mode;
	}
	
	void addRenderable(const OpenGLRenderable& renderable, 
					   const Matrix4& localToWorld) 
	{
		if (m_state_stack.back()._highlight & ePrimitive) {
			_selectedShader->addRenderable(renderable, localToWorld);
		}
		else {
			m_state_stack.back()._state->addRenderable(renderable, localToWorld);
		}
	}

	void render(const Matrix4& modelview, const Matrix4& projection) {
		GlobalShaderCache().render(m_globalstate, modelview, projection);
	}
}; // class XYRenderer

#endif /*XYRENDERER_H_*/
