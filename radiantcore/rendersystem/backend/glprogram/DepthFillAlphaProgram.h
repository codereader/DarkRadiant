#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class DepthFillAlphaProgram :
    public GLSLProgramBase,
    public ISupportsAlphaTest
{
    GLint _locAlphaTest = -1;
    GLint _locObjectTransform = -1;
    GLint _locModelViewProjection = -1;
    GLint _locDiffuseTextureMatrix = -1;

public:
    DepthFillAlphaProgram();

    void enable() override;
    void disable() override;

    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);

    void setAlphaTest(float alphaTest) override;
    void setDiffuseTextureTransform(const Matrix4& transform) override;
};

}
