#include "InteractionProgram.h"
#include "../GLProgramFactory.h"

#include "GLProgramAttributes.h"
#include "itextstream.h"
#include "igame.h"
#include "ishaders.h"
#include "string/convert.h"
#include "debugging/gl.h"
#include "math/Matrix4.h"
#include "../OpenGLState.h"
#include "render/Rectangle.h"
#include "rendersystem/backend/FrameBuffer.h"

namespace render
{

namespace
{
    // Lightscale registry path
    const char* LOCAL_RKEY_LIGHTSCALE = "/defaults/lightScale";

    // Filenames of shader code
    const char* BUMP_VP_FILENAME = "interaction_vp.glsl";
    const char* BUMP_FP_FILENAME = "interaction_fp.glsl";

}

// Main construction
void InteractionProgram::create()
{
	// Initialise the lightScale value
    auto currentGame = GlobalGameManager().currentGame();
    auto scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
    _lightScale = !scaleList.empty() ? string::convert<float>(scaleList[0].getContent()) : 1.0f;

    // Create the program object
    rMessage() << "[renderer] Creating GLSL bump program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        BUMP_VP_FILENAME, BUMP_FP_FILENAME
    );

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, GLProgramAttribute::Position, "attr_Position");
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");
    glBindAttribLocation(_programObj, GLProgramAttribute::Colour, "attr_Colour");
    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the uniform locations to the correct bound values
    _locLocalLightOrigin = glGetUniformLocation(_programObj, "u_LocalLightOrigin");
    _locWorldLightOrigin = glGetUniformLocation(_programObj, "u_WorldLightOrigin");
    _locWorldUpLocal = glGetUniformLocation(_programObj, "u_WorldUpLocal");
    _locLightColour = glGetUniformLocation(_programObj, "u_LightColour");
    _locViewOrigin = glGetUniformLocation(_programObj, "u_LocalViewOrigin");
    _locLightScale = glGetUniformLocation(_programObj, "u_LightScale");
    _locAmbientLight = glGetUniformLocation(_programObj, "u_IsAmbientLight");
    _locColourModulation = glGetUniformLocation(_programObj, "u_ColourModulation");
    _locColourAddition = glGetUniformLocation(_programObj, "u_ColourAddition");
    _locModelViewProjection = glGetUniformLocation(_programObj, "u_ModelViewProjection");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");

    _locDiffuseTextureMatrix = glGetUniformLocation(_programObj, "u_DiffuseTextureMatrix");
    _locBumpTextureMatrix = glGetUniformLocation(_programObj, "u_BumpTextureMatrix");
    _locSpecularTextureMatrix = glGetUniformLocation(_programObj, "u_SpecularTextureMatrix");
    _locLightTextureMatrix = glGetUniformLocation(_programObj, "u_LightTextureMatrix");

    _locShadowMapRect = glGetUniformLocation(_programObj, "u_ShadowMapRect");
    _locUseShadowMap = glGetUniformLocation(_programObj, "u_UseShadowMap");

    // Set up the texture uniforms. The renderer uses fixed texture units for
    // particular textures, so make sure they are correct here.
    // Texture 0 - diffuse
    // Texture 1 - bump
    // Texture 2 - specular
    // Texture 3 - XY attenuation map
    // Texture 4 - Z attenuation map

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    GLint samplerLoc = glGetUniformLocation(_programObj, "u_Diffusemap");
    glUniform1i(samplerLoc, 0);

    samplerLoc = glGetUniformLocation(_programObj, "u_Bumpmap");
    glUniform1i(samplerLoc, 1);

    samplerLoc = glGetUniformLocation(_programObj, "u_Specularmap");
    glUniform1i(samplerLoc, 2);

    samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_xy");
    glUniform1i(samplerLoc, 3);

    samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_z");
    glUniform1i(samplerLoc, 4);

    samplerLoc = glGetUniformLocation(_programObj, "u_ShadowMap");
    glUniform1i(samplerLoc, 5);

    // Light scale is constant at this point
    glUniform1f(_locLightScale, _lightScale);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void InteractionProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArray(GLProgramAttribute::Position);
    glEnableVertexAttribArray(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArray(GLProgramAttribute::Tangent);
    glEnableVertexAttribArray(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArray(GLProgramAttribute::Normal);
    glEnableVertexAttribArray(GLProgramAttribute::Colour);

    debug::assertNoGlErrors();
}

void InteractionProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArray(GLProgramAttribute::Tangent);
    glDisableVertexAttribArray(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArray(GLProgramAttribute::Normal);
    glDisableVertexAttribArray(GLProgramAttribute::Colour);

    // Switch back to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);

    debug::assertNoGlErrors();
}

void InteractionProgram::setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour)
{
    // Define the colour factors to blend into the final fragment
    switch (vertexColourMode)
    {
    case IShaderLayer::VERTEX_COLOUR_NONE:
        // Nullify the vertex colour, add the stage colour as additive constant
        glUniform4f(_locColourModulation, 0, 0, 0, 0);
        glUniform4f(_locColourAddition,
            static_cast<float>(stageColour.x()),
            static_cast<float>(stageColour.y()),
            static_cast<float>(stageColour.z()),
            static_cast<float>(stageColour.w()));
        break;

    case IShaderLayer::VERTEX_COLOUR_MULTIPLY:
        // Multiply the fragment with 1*vertexColour
        glUniform4f(_locColourModulation, 1, 1, 1, 1);
        glUniform4f(_locColourAddition, 0, 0, 0, 0);
        break;

    case IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY:
        // Multiply the fragment with (1 - vertexColour)
        glUniform4f(_locColourModulation, -1, -1, -1, -1);
        glUniform4f(_locColourAddition, 1, 1, 1, 1);
        break;
    }
}

void InteractionProgram::setModelViewProjection(const Matrix4& modelViewProjection)
{
    loadMatrixUniform(_locModelViewProjection, modelViewProjection);
}

void InteractionProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

void InteractionProgram::setDiffuseTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locDiffuseTextureMatrix, transform);
}

void InteractionProgram::setBumpTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locBumpTextureMatrix, transform);
}

void InteractionProgram::setSpecularTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locSpecularTextureMatrix, transform);
}

void InteractionProgram::setupLightParameters(OpenGLState& state, const RendererLight& light, std::size_t renderTime)
{
    // Get the light shader and examine its first (and only valid) layer
    const auto & shader = light.getShader();
    assert(shader);

    const auto& lightMat = shader->getMaterial();
    auto* layer = lightMat ? lightMat->firstLayer() : nullptr;
    if (!layer) return;

    // Calculate all dynamic values in the layer
    layer->evaluateExpressions(renderTime, light.getLightEntity());

    // Get the XY and Z falloff texture numbers.
    auto attenuation_xy = layer->getTexture()->getGLTexNum();
    auto attenuation_z = lightMat->lightFalloffImage()->getGLTexNum();

    // Bind the falloff textures
    OpenGLState::SetTextureState(state.texture3, attenuation_xy, GL_TEXTURE3, GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    OpenGLState::SetTextureState(state.texture4, attenuation_z, GL_TEXTURE4, GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glUniform1i(_locAmbientLight, lightMat->isAmbientLight());
    glUniform3fv(_locLightColour, 1, layer->getColour());

    // Send the light texture transform
    loadMatrixUniform(_locLightTextureMatrix, light.getLightTextureTransformation());
}

void InteractionProgram::setUpObjectLighting(const Vector3& worldLightOrigin,
    const Vector3& viewer,
    const Matrix4& inverseObjectTransform)
{
    debug::assertNoGlErrors();

    const auto& worldToObject = inverseObjectTransform;

    // Calculate the light origin in object space
    auto localLightOrigin = worldToObject.transformPoint(worldLightOrigin);

    // Calculate world up (0,0,1) in object space
    // This is needed for ambient lights
    auto worldUpLocal = worldToObject.zCol3();

    // Calculate viewer location in object space
    auto osViewer = inverseObjectTransform.transformPoint(viewer);

    // Set lighting parameters in the shader
    glUniform3f(_locViewOrigin,
        static_cast<float>(osViewer.x()),
        static_cast<float>(osViewer.y()),
        static_cast<float>(osViewer.z())
    );
    glUniform3f(_locLocalLightOrigin,
        static_cast<float>(localLightOrigin.x()),
        static_cast<float>(localLightOrigin.y()),
        static_cast<float>(localLightOrigin.z())
    );
    glUniform3f(_locWorldLightOrigin,
        static_cast<float>(worldLightOrigin.x()),
        static_cast<float>(worldLightOrigin.y()),
        static_cast<float>(worldLightOrigin.z())
    );
    glUniform3f(_locWorldUpLocal,
        static_cast<float>(worldUpLocal.x()),
        static_cast<float>(worldUpLocal.y()),
        static_cast<float>(worldUpLocal.z())
    );

    debug::assertNoGlErrors();
}

void InteractionProgram::enableShadowMapping(bool enable)
{
    glUniform1i(_locUseShadowMap, enable ? 1 : 0);
    debug::assertNoGlErrors();
}

void InteractionProgram::setShadowMapRectangle(const Rectangle& rectangle)
{
    // Modeled after the TDM code, which is correcting the rectangle to refer to pixel space coordinates
    // idVec4 v( page.x, page.y, 0, page.width-1 );
    // v.ToVec2() = (v.ToVec2() * 2 + idVec2(1, 1)) / (2 * 6 * r_shadowMapSize.GetInteger());
    // v.w /= 6 * r_shadowMapSize.GetFloat();
    auto position = (Vector2f(rectangle.x, rectangle.y) * 2 + Vector2f(1, 1)) / (2 * FrameBuffer::DefaultShadowMapSize);

    glUniform4f(_locShadowMapRect, position.x(), position.y(), 
        0, (static_cast<float>(rectangle.width) - 1) / FrameBuffer::DefaultShadowMapSize);
    debug::assertNoGlErrors();
}

}




