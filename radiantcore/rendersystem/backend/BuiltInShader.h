#pragma once

#include "OpenGLShader.h"

namespace render
{

class BuiltInShader :
    public OpenGLShader
{
private:
    BuiltInShaderType _type;

public:
    BuiltInShader(BuiltInShaderType type, OpenGLRenderSystem& renderSystem);

    static std::string GetNameForType(BuiltInShaderType type);

protected:
    void construct() override;
    void constructWireframeSelectionOverlay(OpenGLState& pass, const std::string& schemeColourKey);
};

}
