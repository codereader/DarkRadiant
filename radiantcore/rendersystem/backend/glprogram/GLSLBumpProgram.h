#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class GLSLBumpProgram :
    public GLSLProgramBase
{
private:
	// The value all lights should be scaled by, obtained from the game description
	float _lightScale;

    // Uniform/program-local parameter IDs.
    int _locLightOrigin;
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

    void setIsAmbientLight(bool isAmbientLight);
    void setLightColour(const Colour4& lightColour);

    // The stage's vertex colour mode and colour as defined by the rgba registers
    void setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour);

    void applyRenderParams(const Vector3& viewer,
        const Matrix4& localToWorld,
        const Params& lightParms) override
    { }

    void setUpLightingCalculation(const Vector3& worldLightOrigin,
        const Matrix4& worldToLight,
        const Vector3& viewer,
        const Matrix4& objectTransform,
        const Matrix4& inverseObjectTransform);
};

} // namespace render

