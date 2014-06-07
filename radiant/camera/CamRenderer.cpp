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

void CamRenderer::SetState(const ShaderPtr& shader, EStyle style)
{
    if(style == eFullMaterials)
    {
        assert(shader);
        _stateStack.back().shader = shader.get();
    }
}

bool CamRenderer::supportsFullMaterials() const
{
    return true;
}

void CamRenderer::PushState()
{
    _stateStack.push_back(_stateStack.back());
}

void CamRenderer::PopState()
{
    ASSERT_MESSAGE(!_stateStack.empty(), "popping empty stack");
    _stateStack.pop_back();
}

void CamRenderer::highlightFaces(bool enable)
{
    _stateStack.back().highlightFaces = enable;
}

void CamRenderer::highlightPrimitives(bool enable)
{
    _stateStack.back().highlightPrimitives = enable;
}

void CamRenderer::setLights(const LightList& lights)
{
    _stateStack.back().lights = &lights;
}

void CamRenderer::addRenderable(const OpenGLRenderable& renderable,
                                const Matrix4& world)
{
    if(_stateStack.back().highlightPrimitives)
    {
        _highlightedPrimitiveShader->addRenderable(
          renderable, world, _stateStack.back().lights
        );
    }

    if(_stateStack.back().highlightFaces)
    {
        _highlightedFaceShader->addRenderable(
            renderable, world, _stateStack.back().lights
        );
    }

    _stateStack.back().shader->addRenderable(
        renderable, world, _stateStack.back().lights
    );
}

void CamRenderer::addRenderable(const OpenGLRenderable& renderable,
                                const Matrix4& world,
                                const IRenderEntity& entity)
{
    if (_stateStack.back().highlightPrimitives)
    {
        _highlightedPrimitiveShader->addRenderable(
            renderable, world, entity, _stateStack.back().lights
        );
    }

    if (_stateStack.back().highlightFaces)
    {
        _highlightedFaceShader->addRenderable(
            renderable, world, entity, _stateStack.back().lights
        );
    }

    _stateStack.back().shader->addRenderable(
        renderable, world, entity, _stateStack.back().lights
    );
}

void CamRenderer::render(const Matrix4& modelview, const Matrix4& projection)
{
    GlobalRenderSystem().render(m_globalstate, modelview, projection, m_viewer);
}
