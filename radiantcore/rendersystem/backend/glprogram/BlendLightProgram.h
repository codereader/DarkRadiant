#pragma once

#include "GLSLProgramBase.h"
#include "render/Colour4.h"

namespace render
{

class BlendLightProgram: public GLSLProgramBase
{
    int _locLightTextureMatrix = -1;
    int _locBlendColour = -1;
    int _locModelViewProjection = -1;
    int _locObjectTransform = -1;

public:
    BlendLightProgram();

    void enable() override;
    void disable() override;

    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);
    void setLightTextureTransform(const Matrix4& transform);
    void setBlendColour(const Colour4& colour);
};

}
