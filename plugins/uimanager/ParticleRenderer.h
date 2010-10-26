#ifndef _PARTICLE_RENDERER_H_
#define _PARTICLE_RENDERER_H_

#include "irender.h"
#include "irenderable.h"
#include "ivolumetest.h"

namespace ui
{

/**
 * greebo: This is a front-end renderer (collecting renderables)
 * for use in the ParticlePreview class. It's basically working
 * as wrapper, passing the OpenGLRenderables to the
 * attached RenderSystem as they are incoming.
 */
class ParticleRenderer :
	public RenderableCollector
{
private:
	// The rendersystem we're passing the OpenGLRenderables to
	RenderSystemPtr _renderSystem;

	// The state stack, empty at start
	typedef std::list<ShaderPtr> StateStack;
	StateStack _stateStack;

public:
	ParticleRenderer(const RenderSystemPtr& renderSystem) :
		_renderSystem(renderSystem)
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
	
	/**
	 * Pop the topmost Shader off the internal stack. This discards the value
	 * without returning it.
	 */
	void PopState()
	{
		if (!_stateStack.empty())
		{
			_stateStack.pop_back();
		}
	}
  
	/**
	 * Set the Shader to be used when rendering any subsequently-submitted
	 * OpenGLRenderable object. This shader remains in effect until it is
	 * changed with a subsequent call to SetState().
	 * 
	 * @param state
	 * The Shader to be used from now on.
	 * 
	 * @param mode
	 * The type of rendering (wireframe or shaded) that this shader should be
    * used for. Individual RenderableCollector subclasses may ignore this method
    * call if it does not use the render mode they are interested in.
	 */
	void SetState(const ShaderPtr& state, EStyle mode)
	{
		assert(!_stateStack.empty());

		_stateStack.back() = state;
	}
	
	/**
	 * Submit an OpenGLRenderable object for rendering when the backend render
	 * pass is conducted. The object will be rendered using the Shader previous-
	 * ly set with SetState().
	 * 
	 * @param renderable
	 * The renderable object to submit.
	 * 
	 * @param world
	 * The local to world transform that should be applied to this object when
	 * it is rendered.
	 */
	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world)
	{
		assert(!_stateStack.empty());

		_stateStack.back()->addRenderable(renderable, world);
	}

	const EStyle getStyle() const 
	{
		return eFullMaterials;
	}
	
	void Highlight(EHighlightMode mode, bool bEnable = true) 
	{
		// Do nothing for now
	}
};

// A minimal volume test implementation, return positive in all test cases
class ParticleVolumeTest :
	public VolumeTest
{
private:
	Matrix4 _viewPort;
	Matrix4 _projection;
	Matrix4 _modelView;

public:
	bool TestPoint(const Vector3& point) const { return true; }

	bool TestLine(const Segment& segment) const { return true; }

	bool TestPlane(const Plane3& plane) const { return true; }

	bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const { return true; }

	VolumeIntersectionValue TestAABB(const AABB& aabb) const { return VOLUME_INSIDE; }

	VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const { return VOLUME_INSIDE; }

	virtual bool fill() const { return true; }

	virtual const Matrix4& GetViewport() const 
	{
		return _viewPort;
	}

	virtual const Matrix4& GetProjection() const
	{
		return _projection;
	}

	virtual const Matrix4& GetModelview() const
	{
		return _modelView;
	}
};

} // namespace

#endif /* _PARTICLE_RENDERER_H_ */
