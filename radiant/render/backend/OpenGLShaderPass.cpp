#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"

#include "math/Matrix4.h"
#include "math/AABB.h"
#include "irender.h"
#include "ishaders.h"
#include "texturelib.h"
#include "iglprogram.h"

#include <boost/foreach.hpp>

#include "debugging/render.h"

namespace render
{

namespace
{

// Bind the given texture to the texture unit, if it is different from the
// current state, then set the current state to the new texture.
inline void setTextureState(GLint& current,
                            const GLint& texture,
                            GLenum textureUnit,
                            GLenum textureMode)
{
    if (texture != current)
    {
        glActiveTexture(textureUnit);
        glClientActiveTexture(textureUnit);
        glBindTexture(textureMode, texture);
        GlobalOpenGL().assertNoErrors();
        current = texture;
    }
}

// Same as setTextureState() above without texture unit parameter
inline void setTextureState(GLint& current,
                            const GLint& texture,
                            GLenum textureMode)
{
    if (texture != current)
    {
        glBindTexture(textureMode, texture);
        GlobalOpenGL().assertNoErrors();
        current = texture;
    }
}

// Utility function to toggle an OpenGL state flag
inline void setState(unsigned int state,
                     unsigned int delta,
                     unsigned int flag,
                     GLenum glflag)
{
    if (delta & state & flag)
    {
        glEnable(glflag);
        GlobalOpenGL().assertNoErrors();
    }
    else if(delta & ~state & flag)
    {
        glDisable(glflag);
        GlobalOpenGL().assertNoErrors();
    }
}

inline void evaluateStage(const ShaderLayerPtr& stage, std::size_t time, const IRenderEntity* entity)
{
    if (stage)
    {
        if (entity)
        {
            stage->evaluateExpressions(time, *entity);
        }
        else
        {
            stage->evaluateExpressions(time);
        }
    }
}

} // namespace

// GL state enabling/disabling helpers

void OpenGLShaderPass::setTexture0()
{
    if (GLEW_VERSION_1_3)
    {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }
}

void OpenGLShaderPass::enableTexture2D()
{
    GlobalOpenGL().assertNoErrors();

    setTexture0();
    glEnable(GL_TEXTURE_2D);

    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::disableTexture2D()
{
    setTexture0();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    GlobalOpenGL().assertNoErrors();
}

// Enable cubemap texturing and texcoord array
void OpenGLShaderPass::enableTextureCubeMap()
{
    setTexture0();
    glEnable(GL_TEXTURE_CUBE_MAP);

    GlobalOpenGL().assertNoErrors();
}

// Disable cubemap texturing and texcoord array
void OpenGLShaderPass::disableTextureCubeMap()
{
    setTexture0();
    glDisable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::enableRenderBlend()
{
    glEnable(GL_BLEND);
    setTexture0();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::disableRenderBlend()
{
    glDisable(GL_BLEND);
    setTexture0();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GlobalOpenGL().assertNoErrors();
}

void OpenGLShaderPass::setupTextureMatrix(GLenum textureUnit, const ShaderLayerPtr& stage)
{
    // Set the texture matrix for the given unit
    glActiveTexture(textureUnit);
    glClientActiveTexture(textureUnit);

    if (stage)
    {
        static const Matrix4 transMinusHalf = Matrix4::getTranslation(Vector3(-0.5f, -0.5f, 0));
        static const Matrix4 transPlusHalf = Matrix4::getTranslation(Vector3(+0.5f, +0.5f, 0));

        Matrix4 tex = Matrix4::getIdentity();

        Vector2 scale = stage->getScale();

        if (stage->getStageFlags() & ShaderLayer::FLAG_CENTERSCALE)
        {
            // Center scale, apply translation by -0.5 first, then scale, then translate back
            tex.multiplyBy(transMinusHalf);
            tex.multiplyBy(Matrix4::getScale(Vector3(scale.x(), scale.y(), 1)));
            tex.multiplyBy(transPlusHalf);
        }
        else
        {
            // Regular scale, apply translation and scale
            tex.multiplyBy(Matrix4::getScale(Vector3(scale.x(), scale.y(), 1)));
        }

        Vector2 shear = stage->getShear();

        if (shear.x() != 0 || shear.y() != 0)
        {
            Matrix4 shearMatrix = Matrix4::byColumns(
                1, shear.y(), 0, 0,
                shear.x(), 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            );

            tex.multiplyBy(transMinusHalf);
            tex.multiplyBy(shearMatrix);
            tex.multiplyBy(transPlusHalf);
        }

        // Rotation
        float rotate = stage->getRotation();

        if (rotate != 0)
        {
            float angle = rotate * 2 * static_cast<float>(c_pi);

            Matrix4 rot = Matrix4::getRotationAboutZ(angle);

            tex.multiplyBy(transMinusHalf);
            tex.multiplyBy(rot);
            tex.multiplyBy(transPlusHalf);
        }

        // Apply translation as last step
        Vector2 translation = stage->getTranslation();
        tex.multiplyBy(Matrix4::getTranslation(Vector3(translation.x(), translation.y(), 0)));

        glLoadMatrixd(tex);
    }
    else
    {
        glLoadMatrixd(Matrix4::getIdentity());
    }
}

// Apply all textures to texture units
void OpenGLShaderPass::applyAllTextures(OpenGLState& current,
                                        unsigned requiredState)
{
    // Set the texture dimensionality from render flags. There is only a global
    // mode for all textures, we can't have texture1 as 2D and texture2 as
    // CUBE_MAP for example.
    GLenum textureMode = 0;

    if (requiredState & RENDER_TEXTURE_CUBEMAP) // cube map has priority
    {
        textureMode = GL_TEXTURE_CUBE_MAP;
    }
    else if (requiredState & RENDER_TEXTURE_2D)
    {
        textureMode = GL_TEXTURE_2D;
    }

    // Apply our texture numbers to the current state
    if (textureMode != 0) // only if one of the RENDER_TEXTURE options
    {
        glMatrixMode(GL_TEXTURE);

        if (GLEW_VERSION_1_3)
        {
            setTextureState(current.texture0, _glState.texture0, GL_TEXTURE0, textureMode);
            setupTextureMatrix(GL_TEXTURE0, _glState.stage0);

            setTextureState(current.texture1, _glState.texture1, GL_TEXTURE1, textureMode);
            setupTextureMatrix(GL_TEXTURE1, _glState.stage1);

            setTextureState(current.texture2, _glState.texture2, GL_TEXTURE2, textureMode);
            setupTextureMatrix(GL_TEXTURE2, _glState.stage2);

            setTextureState(current.texture3, _glState.texture2, GL_TEXTURE2, textureMode);
            setTextureState(current.texture4, _glState.texture2, GL_TEXTURE2, textureMode);

            glActiveTexture(GL_TEXTURE0);
            glClientActiveTexture(GL_TEXTURE0);
        }
        else
        {
            setTextureState(current.texture0, _glState.texture0, textureMode);
            setupTextureMatrix(GL_TEXTURE0, _glState.stage0);
        }

        glMatrixMode(GL_MODELVIEW);
    }
}

// Set up cube map
void OpenGLShaderPass::setUpCubeMapAndTexGen(OpenGLState& current,
                                             unsigned requiredState,
                                             const Vector3& viewer)
{
    if (requiredState & RENDER_TEXTURE_CUBEMAP)
    {
        // Copy cubemap mode enum to current state object
        current.cubeMapMode = _glState.cubeMapMode;

        // Apply axis transformation (swap Y and Z coordinates)
        Matrix4 transform = Matrix4::byRows(
            1, 0, 0, 0,
            0, 0, 1, 0,
            0, 1, 0, 0,
            0, 0, 0, 1
        );

        // Subtract the viewer position
        transform.translateBy(-viewer);

        // Apply to the texture matrix
        glMatrixMode(GL_TEXTURE);
        glLoadMatrixd(transform);
        glMatrixMode(GL_MODELVIEW);
    }
}

// Apply own state to current state object
void OpenGLShaderPass::applyState(OpenGLState& current,
                                  unsigned int globalStateMask,
                                  const Vector3& viewer,
                                  std::size_t time,
                                  const IRenderEntity* entity)
{
    // Evaluate any shader expressions
    if (_glState.stage0)
    {
        evaluateStage(_glState.stage0, time, entity);

        // The alpha test value might change over time
        if (_glState.stage0->getAlphaTest() > 0)
        {
            _glState.setRenderFlag(RENDER_ALPHATEST);
        }
        else
        {
            _glState.clearRenderFlag(RENDER_ALPHATEST);
        }
    }

    if (_glState.stage1) evaluateStage(_glState.stage1, time, entity);
    if (_glState.stage2) evaluateStage(_glState.stage2, time, entity);
    if (_glState.stage3) evaluateStage(_glState.stage3, time, entity);
    if (_glState.stage4) evaluateStage(_glState.stage4, time, entity);

    if (_glState.testRenderFlag(RENDER_OVERRIDE))
    {
        globalStateMask |= RENDER_FILL | RENDER_DEPTHWRITE;
    }

    // Apply the global state mask to our own desired render flags to determine
    // the final set of flags that must bet set
    const unsigned requiredState = _glState.getRenderFlags() & globalStateMask;

    // Construct a mask containing all the flags that will be changing between
    // the current state and the required state. This avoids performing
    // unnecessary GL calls to set the state to its existing value.
    const unsigned changingBitsMask = requiredState ^ current.getRenderFlags();

    // Set the GLProgram if required
    GLProgram* program = (requiredState & RENDER_PROGRAM) != 0
                          ? _glState.glProgram
                          : 0;

    if (program != current.glProgram)
    {
        if (current.glProgram != 0)
        {
            current.glProgram->disable();
            glColor4dv(current.getColour());
        }

        current.glProgram = program;

        if (current.glProgram != 0)
        {
            current.glProgram->enable();
        }
    }

    // State changes. Only perform these if changingBitsMask > 0, since if there are
    // no changes required we don't want a whole load of unnecessary bit
    // operations.
    if (changingBitsMask != 0)
    {
        if(changingBitsMask & requiredState & RENDER_FILL)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_FILL)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            GlobalOpenGL().assertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_OFFSETLINE, GL_POLYGON_OFFSET_LINE);

        if(changingBitsMask & requiredState & RENDER_LIGHTING)
        {
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            glEnableClientState(GL_NORMAL_ARRAY);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_LIGHTING)
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_COLOR_MATERIAL);
            glDisableClientState(GL_NORMAL_ARRAY);
            GlobalOpenGL().assertNoErrors();
        }

        // RENDER_TEXTURE_CUBEMAP
        if(changingBitsMask & requiredState & RENDER_TEXTURE_CUBEMAP)
        {
            enableTextureCubeMap();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_CUBEMAP)
        {
            disableTextureCubeMap();
        }

        // RENDER_TEXTURE_2D
        if(changingBitsMask & requiredState & RENDER_TEXTURE_2D)
        {
            enableTexture2D();
        }
        else if(changingBitsMask & ~requiredState & RENDER_TEXTURE_2D)
        {
            disableTexture2D();
        }

        // RENDER_BLEND
        if(changingBitsMask & requiredState & RENDER_BLEND)
        {
            enableRenderBlend();
        }
        else if(changingBitsMask & ~requiredState & RENDER_BLEND)
        {
            disableRenderBlend();
        }

        setState(requiredState, changingBitsMask, RENDER_CULLFACE, GL_CULL_FACE);

        if(changingBitsMask & requiredState & RENDER_SMOOTH)
        {
            glShadeModel(GL_SMOOTH);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_SMOOTH)
        {
            glShadeModel(GL_FLAT);
            GlobalOpenGL().assertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_SCALED, GL_NORMALIZE); // not GL_RESCALE_NORMAL

        setState(requiredState, changingBitsMask, RENDER_DEPTHTEST, GL_DEPTH_TEST);

        if(changingBitsMask & requiredState & RENDER_DEPTHWRITE)
        {
            glDepthMask(GL_TRUE);

            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_DEPTHWRITE)
        {
            glDepthMask(GL_FALSE);

            GlobalOpenGL().assertNoErrors();
        }

        // Disable colour buffer writes if required
        if(changingBitsMask & requiredState & RENDER_MASKCOLOUR)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            GlobalOpenGL().assertNoErrors();
        }
        else if(changingBitsMask & ~requiredState & RENDER_MASKCOLOUR)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            GlobalOpenGL().assertNoErrors();
        }

        setState(requiredState, changingBitsMask, RENDER_ALPHATEST, GL_ALPHA_TEST);

        // Set GL states corresponding to RENDER_ flags
        setState(requiredState, changingBitsMask, RENDER_LINESTIPPLE, GL_LINE_STIPPLE);
        setState(requiredState, changingBitsMask, RENDER_POLYGONSTIPPLE, GL_POLYGON_STIPPLE);

    } // end of changingBitsMask-dependent changes

    // Set depth function
    if (requiredState & RENDER_DEPTHTEST 
        && _glState.getDepthFunc() != current.getDepthFunc())
    {
        glDepthFunc(_glState.getDepthFunc());
        GlobalOpenGL().assertNoErrors();
        current.setDepthFunc(_glState.getDepthFunc());
    }

  if(requiredState & RENDER_LINESTIPPLE
    && (_glState.m_linestipple_factor != current.m_linestipple_factor
    || _glState.m_linestipple_pattern != current.m_linestipple_pattern))
  {
    glLineStipple(_glState.m_linestipple_factor, _glState.m_linestipple_pattern);
    GlobalOpenGL().assertNoErrors();
    current.m_linestipple_factor = _glState.m_linestipple_factor;
    current.m_linestipple_pattern = _glState.m_linestipple_pattern;
  }

    // Set up the alpha test parameters
    if (requiredState & RENDER_ALPHATEST
        && ( _glState.alphaFunc != current.alphaFunc
            || _glState.alphaThreshold != current.alphaThreshold)
    )
    {
        // Set alpha function in GL
        glAlphaFunc(_glState.alphaFunc, _glState.alphaThreshold);
        GlobalOpenGL().assertNoErrors();

        // Store state values
        current.alphaFunc = _glState.alphaFunc;
        current.alphaThreshold = _glState.alphaThreshold;
    }

    // Apply polygon offset
    if (_glState.polygonOffset != current.polygonOffset)
    {
        current.polygonOffset = _glState.polygonOffset;

        if (current.polygonOffset > 0.0f)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1, -1 *_glState.polygonOffset);
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    // Apply the GL textures
    applyAllTextures(current, requiredState);

    // Set the GL colour. Do this unconditionally, since setting glColor is
    // cheap and it avoids problems with leaked colour states.
    if (_glState.stage0)
    {
        _glState.setColour(_glState.stage0->getColour());
    }
    glColor4dv(_glState.getColour());
    current.setColour(_glState.getColour());
    GlobalOpenGL().assertNoErrors();

    // Set up the cubemap and texgen parameters
    setUpCubeMapAndTexGen(current, requiredState, viewer);

  if(requiredState & RENDER_BLEND
    && (_glState.m_blend_src != current.m_blend_src || _glState.m_blend_dst != current.m_blend_dst))
  {
    glBlendFunc(_glState.m_blend_src, _glState.m_blend_dst);
    GlobalOpenGL().assertNoErrors();
    current.m_blend_src = _glState.m_blend_src;
    current.m_blend_dst = _glState.m_blend_dst;
  }

  if(!(requiredState & RENDER_FILL)
    && _glState.m_linewidth != current.m_linewidth)
  {
    glLineWidth(_glState.m_linewidth);
    GlobalOpenGL().assertNoErrors();
    current.m_linewidth = _glState.m_linewidth;
  }

  if(!(requiredState & RENDER_FILL)
    && _glState.m_pointsize != current.m_pointsize)
  {
    glPointSize(_glState.m_pointsize);
    GlobalOpenGL().assertNoErrors();
    current.m_pointsize = _glState.m_pointsize;
  }

  current.setRenderFlags(requiredState);

  GlobalOpenGL().assertNoErrors();
}

// Add a Renderable to this bucket
void OpenGLShaderPass::addRenderable(const OpenGLRenderable& renderable,
                                      const Matrix4& modelview,
                                      const RendererLight* light)
{
    _renderablesWithoutEntity.push_back(TransformedRenderable(renderable, modelview, light, NULL));
}

void OpenGLShaderPass::addRenderable(const OpenGLRenderable& renderable,
                                      const Matrix4& modelview,
                                      const IRenderEntity& entity,
                                      const RendererLight* light)
{
    RenderablesByEntity::iterator i = _renderables.find(&entity);

    if (i == _renderables.end())
    {
        i = _renderables.insert(RenderablesByEntity::value_type(&entity, Renderables())).first;
    }

    i->second.push_back(TransformedRenderable(renderable, modelview, light, &entity));
}

// Render the bucket contents
void OpenGLShaderPass::render(OpenGLState& current,
                              unsigned int flagsMask,
                              const Vector3& viewer,
                              std::size_t time)
{
    // Reset the texture matrix
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(Matrix4::getIdentity());

    glMatrixMode(GL_MODELVIEW);

    // Apply our state to the current state object
    applyState(current, flagsMask, viewer, time, NULL);

    if (!_renderablesWithoutEntity.empty())
    {
        renderAllContained(_renderablesWithoutEntity, current, viewer, time);
    }

    for (RenderablesByEntity::const_iterator i = _renderables.begin();
         i != _renderables.end();
         ++i)
    {
        // Apply our state to the current state object
        applyState(current, flagsMask, viewer, time, i->first);

        if (!stateIsActive())
        {
            continue;
        }

        renderAllContained(i->second, current, viewer, time);
    }

    _renderablesWithoutEntity.clear();
    _renderables.clear();
}

bool OpenGLShaderPass::stateIsActive()
{
    return ((_glState.stage0 == NULL || _glState.stage0->isVisible()) &&
            (_glState.stage1 == NULL || _glState.stage1->isVisible()) &&
            (_glState.stage2 == NULL || _glState.stage2->isVisible()) &&
            (_glState.stage3 == NULL || _glState.stage3->isVisible()));
}

// Setup lighting
void OpenGLShaderPass::setUpLightingCalculation(OpenGLState& current,
                                                const RendererLight* light,
                                                const Vector3& viewer,
                                                const Matrix4& objTransform,
                                                std::size_t time)
{
    assert(light);

    // Get the light shader and examine its first (and only valid) layer
    const MaterialPtr& lightShader = light->getShader()->getMaterial();
    ShaderLayer* layer = lightShader->firstLayer();
    if (!layer) return;

    // Calculate viewer location in object space
    Matrix4 inverseObjTransform = objTransform.getInverse();
    Vector3 osViewer = inverseObjTransform.transformPoint(viewer);

    // Calculate all dynamic values in the layer
    layer->evaluateExpressions(time, *light);

    // Get the XY and Z falloff texture numbers.
    GLuint attenuation_xy = layer->getTexture()->getGLTexNum();
    GLuint attenuation_z = lightShader->lightFalloffImage()->getGLTexNum();

    // Bind the falloff textures
    assert(current.testRenderFlag(RENDER_TEXTURE_2D));

    setTextureState(
        current.texture3, attenuation_xy, GL_TEXTURE3, GL_TEXTURE_2D
    );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    setTextureState(
        current.texture4, attenuation_z, GL_TEXTURE4, GL_TEXTURE_2D
    );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Get the world-space to light-space transformation matrix
    Matrix4 world2light = light->getLightTextureTransformation();

    // Set the GL program parameters
    GLProgram::Params parms(
        light->getLightOrigin(), layer->getColour(), world2light
    );
    parms.ambientFactor = lightShader->isAmbientLight() ? 1.0f : 0.0f;
    parms.invertVertexColour = _glState.isColourInverted();

    assert(current.glProgram);
    current.glProgram->applyRenderParams(osViewer, objTransform, parms);
}

// Flush renderables
void OpenGLShaderPass::renderAllContained(const Renderables& renderables,
                                          OpenGLState& current,
                                          const Vector3& viewer,
                                          std::size_t time)
{
    // Keep a pointer to the last transform matrix and render entity used
    const Matrix4* transform = 0;

    glPushMatrix();

    // Iterate over each transformed renderable in the vector
    BOOST_FOREACH (const TransformedRenderable& r, renderables)
    {
        // If the current iteration's transform matrix was different from the
        // last, apply it and store for the next iteration
        if (transform == NULL ||
            (transform != r.transform && !transform->isAffineEqual(*r.transform)))
        {
            transform = r.transform;
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

        // If we are using a lighting program and this renderable is lit, set
        // up the lighting calculation
        const RendererLight* light = r.light;
        if (current.glProgram && light)
        {
            setUpLightingCalculation(current, light, viewer, *transform, time);
        }

        // Render the renderable
        RenderInfo info(current.getRenderFlags(), viewer, current.cubeMapMode);
        r.renderable->render(info);
    }

    // Cleanup
    glPopMatrix();
}

// Stream insertion operator
std::ostream& operator<<(std::ostream& st, const OpenGLShaderPass& self)
{
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
