#pragma once

#include "irenderable.h"

class XYRenderer :
	public RenderableCollector
{
	// State type structure
	struct State
	{
		bool highlightPrimitives;
		bool highlightAsGroupMember;
		Shader* shader;

		// Constructor
		State() : 
			highlightPrimitives(false), 
			highlightAsGroupMember(false),
			shader(nullptr)
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

#if 0
	void SetState(const ShaderPtr& state, EStyle style)
	{
		if (style == eWireframeOnly)
		{
			ASSERT_NOTNULL(state);
			_stateStack.back().shader = state.get();
		}
	}
#endif

	bool supportsFullMaterials() const
    {
		return false;
	}

#if 0
	void PushState() {
		// Duplicate the most recent state
		_stateStack.push_back(_stateStack.back());
	}

	void PopState() {
		_stateStack.pop_back();
	}
#endif

	void setHighlightFlag(Highlight::Flags flags, bool enabled)
	{
		if (flags & Highlight::Primitives)
		{
			_stateStack.back().highlightPrimitives = enabled;
		}

		if (flags & Highlight::GroupMember)
		{
			_stateStack.back().highlightAsGroupMember = enabled;
		}
	}

	void addRenderable(const OpenGLRenderable& renderable,
					   const Matrix4& localToWorld)
	{
		if (_stateStack.back().highlightPrimitives)
		{
			if (_stateStack.back().highlightAsGroupMember)
			{
				_selectedShaderGroup->addRenderable(renderable, localToWorld);
			}
			else
			{
				_selectedShader->addRenderable(renderable, localToWorld);
			}
		}
		else if (_stateStack.back().shader != nullptr)
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
			if (_stateStack.back().highlightAsGroupMember)
			{
				_selectedShaderGroup->addRenderable(renderable, localToWorld, entity);
			}
			else
			{
				_selectedShader->addRenderable(renderable, localToWorld, entity);
			}
		}
		else if (_stateStack.back().shader != nullptr)
		{
			_stateStack.back().shader->addRenderable(renderable, localToWorld, entity);
		}
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable, const Matrix4& world) override
	{
		if (_stateStack.back().highlightPrimitives)
		{
			if (_stateStack.back().highlightAsGroupMember)
			{
				_selectedShaderGroup->addRenderable(renderable, world);
			}
			else
			{
				_selectedShader->addRenderable(renderable, world);
			}
		}

		shader->addRenderable(renderable, world);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity) override
	{
		if (_stateStack.back().highlightPrimitives)
		{
			if (_stateStack.back().highlightAsGroupMember)
			{
				_selectedShaderGroup->addRenderable(renderable, world, entity);
			}
			else
			{
				_selectedShader->addRenderable(renderable, world, entity);
			}
		}

		shader->addRenderable(renderable, world, entity);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity, const LightList& lights) override
	{
		if (_stateStack.back().highlightPrimitives)
		{
			if (_stateStack.back().highlightAsGroupMember)
			{
				_selectedShaderGroup->addRenderable(renderable, world, entity, &lights);
			}
			else
			{
				_selectedShader->addRenderable(renderable, world, entity, &lights);
			}
		}

		shader->addRenderable(renderable, world, entity, &lights);
	}

	void render(const Matrix4& modelview, const Matrix4& projection)
    {
		GlobalRenderSystem().render(_globalstate, modelview, projection);
	}
}; // class XYRenderer
