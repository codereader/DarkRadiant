#pragma once

#include "OpenGLShader.h"

namespace render
{

/**
 * Shader used to draw single-coloured items like all map elements
 * in orthoview, target line connectors in both views, speaker and
 * light volumes, etc.
 */
class ColourShader :
    public OpenGLShader
{
private:
    ColourShaderType _type;
    Colour4 _colour;

public:
    ColourShader(ColourShaderType type, const Colour4& colour, OpenGLRenderSystem& renderSystem);

    ColourShaderType getType() const;

    static std::string ConstructName(ColourShaderType type, const Colour4& colour);

protected:
    virtual void construct() override;

    virtual void onMergeModeChanged() override;
};

}
