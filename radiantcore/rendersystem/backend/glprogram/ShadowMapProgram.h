#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class ShadowMapProgram :
    public GLSLProgramBase,
    public ISupportsAlphaTest
{
private:
    GLint _locAlphaTest;
    GLint _locLightOrigin;
    GLint _locObjectTransform;
    GLint _locDiffuseTextureMatrix;

public:
    void create() override;
    void enable() override;
    void disable() override;

    void setObjectTransform(const Matrix4& transform);

    void setDiffuseTextureTransform(const Matrix4& transform) override;
    void setAlphaTest(float alphaTest) override;
    void setLightOrigin(const Vector3& lightOrigin);
};

}
