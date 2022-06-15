#pragma once

#include "GLSLProgramBase.h"

namespace render
{

class RegularStageProgram :
    public GLSLProgramBase
{
private:
    int _locDiffuseTextureMatrix;
    int _locColourModulation;
    int _locColourAddition;
    int _locModelViewProjection;
    int _locObjectTransform;

public:
    RegularStageProgram() :
        _locDiffuseTextureMatrix(-1),
        _locColourModulation(-1),
        _locColourAddition(-1),
        _locModelViewProjection(-1),
        _locObjectTransform(-1)
    {}

    void create() override;
    void enable() override;
    void disable() override;

    void setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour);
    void setModelViewProjection(const Matrix4& modelViewProjection);
    void setObjectTransform(const Matrix4& transform);
    void setDiffuseTextureTransform(const Matrix4& transform);;
};

} // namespace render

