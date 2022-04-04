#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class DepthFillAlphaProgram :
    public GLSLProgramBase,
    public ISupportsAlphaTest
{
private:
    GLint _locAlphaTest;
    GLint _locObjectTransform;
    GLint _locModelViewProjection;
    GLint _locDiffuseTextureMatrix;
    
public:
    void create() override;
    void enable() override;
    void disable() override;

    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);

    void setAlphaTest(float alphaTest) override;
    void setDiffuseTextureTransform(const Matrix4& transform) override;
};

}
