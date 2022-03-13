#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"

#include "math/Matrix4.h"
#include "math/AABB.h"
#include "irender.h"
#include "ishaders.h"
#include "texturelib.h"
#include "iglprogram.h"

#include "debugging/render.h"
#include "debugging/gl.h"

#include "glprogram/GLSLDepthFillAlphaProgram.h"

namespace render
{

namespace
{

inline void evaluateStage(const IShaderLayer::Ptr& stage, std::size_t time, const IRenderEntity* entity)
{
    if (!stage) return;

    if (entity)
    {
        stage->evaluateExpressions(time, *entity);
    }
    else
    {
        stage->evaluateExpressions(time);
    }
}

} // namespace

void OpenGLShaderPass::evaluateShaderStages(std::size_t time, const IRenderEntity* entity)
{
    // Evaluate shader expressions in all stages
    evaluateStage(_glState.stage0, time, entity);
    evaluateStage(_glState.stage1, time, entity);
    evaluateStage(_glState.stage2, time, entity);
    evaluateStage(_glState.stage3, time, entity);
    evaluateStage(_glState.stage4, time, entity);
}

void OpenGLShaderPass::evaluateStagesAndApplyState(OpenGLState& current,
    unsigned int globalStateMask, std::size_t time, const IRenderEntity* entity)
{
    evaluateShaderStages(time, entity);
    applyState(current, globalStateMask);
}

void OpenGLShaderPass::applyState(OpenGLState& current, unsigned int globalStateMask)
{
    // The alpha test value might change over time
    if (_glState.stage0 && _glState.stage0->getAlphaTest() > 0)
    {
        _glState.setRenderFlag(RENDER_ALPHATEST);
    }
    else
    {
        _glState.clearRenderFlag(RENDER_ALPHATEST);
    }

    if (_glState.testRenderFlag(RENDER_OVERRIDE))
    {
        globalStateMask |= RENDER_FILL | RENDER_DEPTHWRITE;
    }

    // Calculate the difference between this state and the current one
    // and apply the changes required to match our required flags.
    _glState.applyTo(current, globalStateMask);
}

void OpenGLShaderPass::addRenderable(const OpenGLRenderable& renderable,
                                     const Matrix4& modelview)
{
    _transformedRenderables.emplace_back(renderable, modelview);
}

// Render the bucket contents
void OpenGLShaderPass::submitSurfaces(OpenGLState& current,
                              unsigned int flagsMask,
                              const VolumeTest& view,
                              std::size_t time)
{
    // Apply our state to the current state object
    evaluateStagesAndApplyState(current, flagsMask, time, nullptr);

    _owner.drawSurfaces(view);
}

void OpenGLShaderPass::submitRenderables(OpenGLState& current, unsigned int flagsMask, std::size_t time)
{
    // Apply our state to the current state object
    evaluateStagesAndApplyState(current, flagsMask, time, nullptr);

    drawRenderables(current);
}

void OpenGLShaderPass::clearRenderables()
{
    _transformedRenderables.clear();
}

bool OpenGLShaderPass::empty()
{
    return _transformedRenderables.empty() && !_owner.hasSurfaces() && !_owner.hasWindings();
}

bool OpenGLShaderPass::hasRenderables() const
{
    return !_transformedRenderables.empty();
}

bool OpenGLShaderPass::isApplicableTo(RenderViewType renderViewType) const
{
    return _owner.isApplicableTo(renderViewType);
}

bool OpenGLShaderPass::stateIsActive()
{
    return ((_glState.stage0 == NULL || _glState.stage0->isVisible()) &&
            (_glState.stage1 == NULL || _glState.stage1->isVisible()) &&
            (_glState.stage2 == NULL || _glState.stage2->isVisible()) &&
            (_glState.stage3 == NULL || _glState.stage3->isVisible()));
}

// Setup lighting
void OpenGLShaderPass::SetUpLightingCalculation(OpenGLState& current,
                                                const RendererLight* light,
                                                const Matrix4& worldToLight,
                                                const Vector3& viewer,
                                                const Matrix4& objTransform,
                                                std::size_t time)
{
    // Get the light shader and examine its first (and only valid) layer
    assert(light);
    const auto& shader = light->getShader();
    assert(shader);

    const MaterialPtr& lightMat = shader->getMaterial();
    IShaderLayer* layer = lightMat ? lightMat->firstLayer() : nullptr;
    if (!layer) return;

    // Calculate viewer location in object space
    auto inverseObjTransform = objTransform.getInverse();
    auto osViewer = inverseObjTransform.transformPoint(viewer);

    // Calculate all dynamic values in the layer
    layer->evaluateExpressions(time, light->getLightEntity());

    // Get the XY and Z falloff texture numbers.
    auto attenuation_xy = layer->getTexture()->getGLTexNum();
    auto attenuation_z = lightMat->lightFalloffImage()->getGLTexNum();

    // Bind the falloff textures
    assert(current.testRenderFlag(RENDER_TEXTURE_2D));

    OpenGLState::SetTextureState(
        current.texture3, attenuation_xy, GL_TEXTURE3, GL_TEXTURE_2D
    );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    OpenGLState::SetTextureState(
        current.texture4, attenuation_z, GL_TEXTURE4, GL_TEXTURE_2D
    );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set the GL program parameters
    GLProgram::Params parms(
        current.getVertexColourMode(),
        current.getColour(),
        light->getLightOrigin(),
        layer->getColour(),
        worldToLight
    );
    parms.isAmbientLight = lightMat->isAmbientLight();

    assert(current.glProgram);
    current.glProgram->applyRenderParams(osViewer, objTransform, parms);
}

void OpenGLShaderPass::SetUpNonInteractionProgram(OpenGLState& current, const Vector3& viewer, const Matrix4& objTransform)
{
    static GLProgram::Params parms(IShaderLayer::VERTEX_COLOUR_NONE, { 1, 1, 1, 1 }, { 0, 0, 0 }, { 0,0,0,0 }, Matrix4::getIdentity());

    assert(current.glProgram);
    current.glProgram->applyRenderParams(viewer, objTransform, parms);
}

void OpenGLShaderPass::drawRenderables(OpenGLState& current)
{
    if (_transformedRenderables.empty()) return;

    // Keep a pointer to the last transform matrix used
    const Matrix4* transform = nullptr;

    glPushMatrix();

    // Iterate over each transformed renderable in the vector
    for (const auto& r : _transformedRenderables)
    {
        // If the current iteration's transform matrix was different from the
        // last, apply it and store for the next iteration
        if (!transform || !transform->isAffineEqual(r.transform))
        {
            transform = &r.transform;
            glPopMatrix();
            glPushMatrix();
            glMultMatrixd(*transform);

            // Determine the face direction
            if (current.testRenderFlag(RENDER_CULLFACE)
                && transform->getHandedness() == Matrix4::RIGHTHANDED)
            {
                glFrontFace(GL_CW);
            }
            else
            {
                glFrontFace(GL_CCW);
            }
        }

        r.renderable->render();
    }

    // Cleanup
    glPopMatrix();
}

// Stream insertion operator
std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self)
{
	if (!self.state().getName().empty())
	{
		st << "Name: " << self.state().getName() << ": ";
	}

    const MaterialPtr& material = self._owner.getMaterial();

    st << (material ? material->getName() : "null material") << " - ";

    st << "Renderflags: " << debug::StateFlagsInserter(self._glState.getRenderFlags());

    st << " - ";

    st << "Sort: " << self._glState.getSortPosition() << " - ";
    st << "PolygonOffset: " << self._glState.polygonOffset << " - ";

    if (self._glState.texture0 > 0) st << "Texture0: " << self._glState.texture0 << " - ";
    if (self._glState.texture1 > 0) st << "Texture1: " << self._glState.texture1 << " - ";
    if (self._glState.texture2 > 0) st << "Texture2: " << self._glState.texture2 << " - ";
    if (self._glState.texture3 > 0) st << "Texture3: " << self._glState.texture3 << " - ";
    if (self._glState.texture4 > 0) st << "Texture4: " << self._glState.texture4 << " - ";

    st << "Colour: " << self._glState.getColour() << " - ";

    st << "CubeMapMode: " << self._glState.cubeMapMode;

    st << std::endl;

    return st;
}

}
