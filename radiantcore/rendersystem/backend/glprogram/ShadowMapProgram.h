#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class ShadowMapProgram :
    public GLSLProgramBase,
    public ISupportsAlphaTest
{
    GLint _locAlphaTest = -1;
    GLint _locLightOrigin = -1;
    GLint _locObjectTransform = -1;
    GLint _locDiffuseTextureMatrix = -1;

public:
    ShadowMapProgram();

    void enable() override;
    void disable() override;

    void setObjectTransform(const Matrix4& transform);

    void setDiffuseTextureTransform(const Matrix4& transform) override;
    void setAlphaTest(float alphaTest) override;
    void setLightOrigin(const Vector3& lightOrigin);
};

}
