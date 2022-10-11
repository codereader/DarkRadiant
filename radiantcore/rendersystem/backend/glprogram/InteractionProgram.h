#pragma once

#include "ishaderlayer.h"
#include "irender.h"
#include "GLSLProgramBase.h"

namespace render
{

class OpenGLState;
struct Rectangle;

class InteractionProgram :
    public GLSLProgramBase
{
private:
	// The value all lights should be scaled by, obtained from the game description
	float _lightScale;

    // Uniform/program-local parameter IDs.
    int _locLocalLightOrigin;
    int _locWorldLightOrigin;
    int _locWorldUpLocal;
    int _locLightColour;
    int _locViewOrigin;
    int _locLightScale;
    int _locAmbientLight;
    int _locColourModulation;
    int _locColourAddition;
    int _locModelViewProjection;
    int _locObjectTransform;

    int _locDiffuseTextureMatrix;
    int _locBumpTextureMatrix;
    int _locSpecularTextureMatrix;
    int _locLightTextureMatrix;

    int _locUseShadowMap;
    int _locShadowMapRect;

public:

    /* GLProgram implementation */
    void create() override;
    void enable() override;
    void disable() override;

    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);

    void setDiffuseTextureTransform(const Matrix4& transform);
    void setBumpTextureTransform(const Matrix4& transform);
    void setSpecularTextureTransform(const Matrix4& transform);

    // The stage's vertex colour mode and colour as defined by the rgba registers
    void setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour);

    void setupLightParameters(OpenGLState& state, const RendererLight& light, std::size_t renderTime);

    void setUpObjectLighting(const Vector3& worldLightOrigin,
        const Vector3& viewer,
        const Matrix4& inverseObjectTransform);

    void setShadowMapRectangle(const Rectangle& rectangle);
    void enableShadowMapping(bool enable);
};

} // namespace render

