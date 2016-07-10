#pragma once

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
	Shader* _selectedShaderGroup;

public:
	XYRenderer(RenderStateFlags globalstate, Shader* selected, Shader* selectedGroup) :
		_globalstate(globalstate),
		_selectedShader(selected),
		_selectedShaderGroup(selectedGroup)
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

	void setHighlightFlag(Highlight::Flags flags, bool enabled)
	{
		if (flags & Highlight::Primitives)
		{
			_stateStack.back().highlightPrimitives = enabled;
		}
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
