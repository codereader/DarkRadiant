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

		// Constructor
		State() : 
			highlightPrimitives(false), 
			highlightAsGroupMember(false)
		{}
	};

	State _state;
	RenderStateFlags _globalstate;

	// Shader to use for highlighted objects
	Shader* _selectedShader;
	Shader* _selectedShaderGroup;

public:
	XYRenderer(RenderStateFlags globalstate, Shader* selected, Shader* selectedGroup) :
		_globalstate(globalstate),
		_selectedShader(selected),
		_selectedShaderGroup(selectedGroup)
	{}

	bool supportsFullMaterials() const override
    {
		return false;
	}

	void setHighlightFlag(Highlight::Flags flags, bool enabled) override
	{
		if (flags & Highlight::Primitives)
		{
			_state.highlightPrimitives = enabled;
		}

		if (flags & Highlight::GroupMember)
		{
			_state.highlightAsGroupMember = enabled;
		}
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable, const Matrix4& world) override
	{
		if (_state.highlightPrimitives)
		{
			if (_state.highlightAsGroupMember)
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
		if (_state.highlightPrimitives)
		{
			if (_state.highlightAsGroupMember)
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
		if (_state.highlightPrimitives)
		{
			if (_state.highlightAsGroupMember)
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
