#pragma once

#include "GLSLProgramBase.h"
#include "render/Colour4.h"

namespace render
{

class BlendLightProgram :
    public GLSLProgramBase
{
private:
    int _locLightTextureMatrix;
    int _locBlendColour;
    int _locModelViewProjection;
    int _locObjectTransform;

public:
    void create() override;
    void enable() override;
    void disable() override;

    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);
    void setLightTextureTransform(const Matrix4& transform);
    void setBlendColour(const Colour4& colour);
};

}
