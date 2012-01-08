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
	// The state stack, empty at start
	typedef std::list<ShaderPtr> StateStack;
	StateStack _stateStack;

public:
	ShaderStateRenderer()
	{
		// Start with an empty shader, which can be assigned in SetState
		_stateStack.push_back(ShaderPtr());
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

		_stateStack.back() = state;
	}

	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world)
	{
		assert(!_stateStack.empty());

		_stateStack.back()->addRenderable(renderable, world);
	}

	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world, const IRenderEntity& entity)
	{
		assert(!_stateStack.empty());

		_stateStack.back()->addRenderable(renderable, world, entity);
	}

	bool supportsFullMaterials() const
	{
        return true;
	}

    // No support for selection highlighting
	void highlightFaces(bool enable = true) { }
	void highlightPrimitives(bool enable = true) { }
};

} // namespace
