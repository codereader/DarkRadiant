#ifndef XYRENDERER_H_
#define XYRENDERER_H_

#include "irenderable.h"

class XYRenderer :
	public RenderableCollector
{
	// State type structure
	struct State
	{
		bool highlightPrimitives;
		Shader* shader;

		// Constructor
		State()
		: highlightPrimitives(false), shader(NULL)
		{}
	};

	std::vector<State> _stateStack;
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

		_stateStack.push_back(State());
	}

	void SetState(const ShaderPtr& state, EStyle style)
	{
		if (style == eWireframeOnly)
		{
			ASSERT_NOTNULL(state);
			_stateStack.back().shader = state.get();
		}
	}

	bool supportsFullMaterials() const
    {
		return false;
	}

	void PushState() {
		// Duplicate the most recent state
		_stateStack.push_back(_stateStack.back());
	}

	void PopState() {
		_stateStack.pop_back();
	}

    void highlightFaces(bool enable = true) { }

    void highlightPrimitives(bool enable = true)
	{
        _stateStack.back().highlightPrimitives = enable;
	}

	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& localToWorld)
	{
		if (_stateStack.back().highlightPrimitives)
		{
			_selectedShader->addRenderable(renderable, localToWorld);
		}
		else if (_stateStack.back().shader != NULL)
		{
			_stateStack.back().shader->addRenderable(renderable, localToWorld);
		}
	}

	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& localToWorld,
					   const IRenderEntity& entity)
	{
		if (_stateStack.back().highlightPrimitives)
		{
			_selectedShader->addRenderable(renderable, localToWorld, entity);
		}
		else if (_stateStack.back().shader != NULL)
		{
			_stateStack.back().shader->addRenderable(renderable, localToWorld, entity);
		}
	}

	void render(const Matrix4& modelview, const Matrix4& projection)
    {
		GlobalRenderSystem().render(_globalstate, modelview, projection);
	}
}; // class XYRenderer

#endif /*XYRENDERER_H_*/
