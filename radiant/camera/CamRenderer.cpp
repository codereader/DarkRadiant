#include "CamRenderer.h"

#include "debugging/debugging.h"

CamRenderer::CamRenderer(RenderStateFlags globalstate,
                         const ShaderPtr& primitiveShader,
                         const ShaderPtr& faceShader,
                         const Vector3& viewer) : 
    m_globalstate(globalstate),
    _highlightedPrimitiveShader(primitiveShader),
    _highlightedFaceShader(faceShader),
    m_viewer(viewer)
{
    assert(primitiveShader);
    assert(faceShader);
}

bool CamRenderer::supportsFullMaterials() const
{
    return true;
}

void CamRenderer::setHighlightFlag(Highlight::Flags flags, bool enabled)
{
    if (flags & Highlight::Faces)
    {
        _state.highlightFaces = enabled;
    }

    if (flags & Highlight::Primitives)
    {
        _state.highlightPrimitives = enabled;
    }
}

void CamRenderer::addRenderable(const ShaderPtr& shader,
                                const OpenGLRenderable& renderable,
                                const Matrix4& world, const LightSources* lights,
                                const IRenderEntity* entity)
{
    if (_state.highlightPrimitives)
        _highlightedPrimitiveShader->addRenderable(renderable, world,
                                                   lights, entity);

    if (_state.highlightFaces)
        _highlightedFaceShader->addRenderable(renderable, world,
                                              lights, entity);

    shader->addRenderable(renderable, world, lights, entity);
}

void CamRenderer::render(const Matrix4& modelview, const Matrix4& projection)
{
    GlobalRenderSystem().render(m_globalstate, modelview, projection, m_viewer);
}
