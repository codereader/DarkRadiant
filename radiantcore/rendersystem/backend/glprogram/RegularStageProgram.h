#pragma once

#include "ishaderlayer.h"
#include "GLSLProgramBase.h"

namespace render
{

class RegularStageProgram :
    public GLSLProgramBase
{
private:
    int _locDiffuseTextureMatrix = -1;
    int _locColourModulation = -1;
    int _locColourAddition = -1;
    int _locModelViewProjection = -1;
    int _locObjectTransform = -1;

public:
    RegularStageProgram();

    void enable() override;
    void disable() override;

    void setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour);
    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);
    void setDiffuseTextureTransform(const Matrix4& transform);
};

} // namespace render
