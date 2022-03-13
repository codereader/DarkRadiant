#pragma once

#include "GLProgramAttributes.h"
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

public:

    /* GLProgram implementation */
    void create() override;
    void enable() override;
    void disable() override;

    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);

    void setDiffuseTextureTransform(const Matrix4& transform);

    void applyRenderParams(const Vector3& viewer,
                           const Matrix4& localToWorld,
                           const Params&) override;
};

} // namespace render

