#ifndef XYRENDERER_H_
#define XYRENDERER_H_

#include "irenderable.h"

class XYRenderer : 
	public RenderableCollector
{
	// State type structure
	struct state_type
	{
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
	
	std::vector<state_type> _stateStack;
	RenderStateFlags _globalstate;
	
	// Shader to use for highlighted objects
	Shader* _selectedShader;

public:
	XYRenderer(RenderStateFlags globalstate, Shader* selected) :
			_globalstate(globalstate),
			_selectedShader(selected) 
	{
		// Reserve space in the vector to avoid reallocation delays
		_stateStack.reserve(8);
		
		_stateStack.push_back(state_type());
	}

	void SetState(const ShaderPtr& state, EStyle style)
	{
		if (style == eWireframeOnly)
		{
			ASSERT_NOTNULL(state);
			_stateStack.back()._state = state.get();
		}
	}
	
	const EStyle getStyle() const {
		return eWireframeOnly;
	}
	
	void PushState() {
		// Duplicate the most recent state
		_stateStack.push_back(_stateStack.back());
	}
	
	void PopState() {
		_stateStack.pop_back();
	}
	
	void Highlight(EHighlightMode mode, bool bEnable = true)
	{
		if (bEnable) 
		{
			_stateStack.back()._highlight |= mode;
		}
		else
		{
			_stateStack.back()._highlight &= ~mode;
		}
	}
	
	void addRenderable(const OpenGLRenderable& renderable, 
					   const Matrix4& localToWorld) 
	{
		if (_stateStack.back()._highlight & ePrimitive) {
			_selectedShader->addRenderable(renderable, localToWorld);
		}
		else {
			_stateStack.back()._state->addRenderable(renderable, localToWorld);
		}
	}

	void render(const Matrix4& modelview, const Matrix4& projection) {
		GlobalRenderSystem().render(_globalstate, modelview, projection);
	}
}; // class XYRenderer

#endif /*XYRENDERER_H_*/
