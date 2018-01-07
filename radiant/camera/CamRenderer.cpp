#include "CamRenderer.h"

#include "debugging/debugging.h"

CamRenderer::CamRenderer(RenderStateFlags globalstate,
                         const ShaderPtr& primitiveShader,
                         const ShaderPtr& faceShader,
                         const Vector3& viewer)
: m_globalstate(globalstate),
  _highlightedPrimitiveShader(primitiveShader),
  _highlightedFaceShader(faceShader),
  m_viewer(viewer)
{
    assert(primitiveShader);
    assert(faceShader);

    // avoid reallocations
    _stateStack.reserve(10);
    _stateStack.push_back(State());
}

#if 0
void CamRenderer::SetState(const ShaderPtr& shader, EStyle style)
{
    if(style == eFullMaterials)
    {
        assert(shader);
        _stateStack.back().shader = shader.get();
    }
}
#endif

bool CamRenderer::supportsFullMaterials() const
{
    return true;
}

#if 0
void CamRenderer::PushState()
{
    _stateStack.push_back(_stateStack.back());
}

void CamRenderer::PopState()
{
    ASSERT_MESSAGE(!_stateStack.empty(), "popping empty stack");
    _stateStack.pop_back();
}
#endif

void CamRenderer::setHighlightFlag(Highlight::Flags flags, bool enabled)
{
	if (flags & Highlight::Faces)
	{
		_stateStack.back().highlightFaces = enabled;
	}

	if (flags & Highlight::Primitives)
	{
		_stateStack.back().highlightPrimitives = enabled;
	}
}

void CamRenderer::setLights(const LightList& lights)
{
    _stateStack.back().lights = &lights;
}

void CamRenderer::addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable, const Matrix4& world)
{
	if (_stateStack.back().highlightPrimitives)
	{
		_highlightedPrimitiveShader->addRenderable(renderable, world);
	}

	if (_stateStack.back().highlightFaces)
	{
		_highlightedFaceShader->addRenderable(renderable, world);
	}

	shader->addRenderable(renderable, world);
}

void CamRenderer::addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
	const Matrix4& world, const IRenderEntity& entity)
{
	if (_stateStack.back().highlightPrimitives)
	{
		_highlightedPrimitiveShader->addRenderable(renderable, world, entity);
	}

	if (_stateStack.back().highlightFaces)
	{
		_highlightedFaceShader->addRenderable(renderable, world, entity);
	}

	shader->addRenderable(renderable, world, entity);
}

void CamRenderer::addRenderable(const ShaderPtr& shader, const OpenGLRenderable& renderable,
	const Matrix4& world, const IRenderEntity& entity, const LightList& lights)
{
	if (_stateStack.back().highlightPrimitives)
	{
		_highlightedPrimitiveShader->addRenderable(renderable, world, entity, &lights);
	}

	if (_stateStack.back().highlightFaces)
	{
		_highlightedFaceShader->addRenderable(renderable, world, entity, &lights);
	}

	shader->addRenderable(renderable, world, entity, &lights);
}

void CamRenderer::render(const Matrix4& modelview, const Matrix4& projection)
{
    GlobalRenderSystem().render(m_globalstate, modelview, projection, m_viewer);
}
