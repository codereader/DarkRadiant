#pragma once

#include "irender.h"
#include <list>

namespace render
{

/**
 * greebo: This is a basic front-end renderer (collecting renderables)
 * using a state stack to sort the renderables. Each renderable
 * is assigned to the topmost shader on the stack. No highlighting support.
 * It is returning FullMaterials as renderer style.
 */
class ShaderStateRenderer :
	public RenderableCollector
{
private:
    struct State
    {
        ShaderPtr shader;
        const LightList* lights;

        State() : 
            lights(nullptr)
        {}
    };

	// The state stack, empty at start
	typedef std::list<State> StateStack;
	StateStack _stateStack;

public:
	ShaderStateRenderer()
	{
		// Start with an empty shader, which can be assigned in SetState
        _stateStack.push_back(State());
	}

	void PushState()
	{
		if (!_stateStack.empty())
		{
			_stateStack.push_back(_stateStack.back());
		}
	}

	void PopState()
	{
		if (!_stateStack.empty())
		{
			_stateStack.pop_back();
		}
	}

	void SetState(const ShaderPtr& state, EStyle mode)
	{
		assert(!_stateStack.empty());

        _stateStack.back().shader = state;
	}

	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world)
	{
		assert(!_stateStack.empty());

        _stateStack.back().shader->addRenderable(renderable, world, _stateStack.back().lights);
	}

	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world, const IRenderEntity& entity)
	{
		assert(!_stateStack.empty());

        _stateStack.back().shader->addRenderable(renderable, world, entity, _stateStack.back().lights);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable, const Matrix4& world) override
	{
		shader->addRenderable(renderable, world);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity) override
	{
		shader->addRenderable(renderable, world, entity);
	}

	void addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
		const Matrix4& world, const IRenderEntity& entity, const LightList& lights) override
	{
		shader->addRenderable(renderable, world, entity, &lights);
	}

	bool supportsFullMaterials() const
	{
        return true;
	}

    // No support for selection highlighting
	void setHighlightFlag(Highlight::Flags flags, bool enabled) {}
	
    void setLights(const LightList& lights)
    {
        _stateStack.back().lights = &lights;
    }
};

} // namespace
