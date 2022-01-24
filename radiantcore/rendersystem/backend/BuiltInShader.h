#pragma once

#include "OpenGLShader.h"

namespace render
{

class BuiltInShader :
    public OpenGLShader
{
public:
    BuiltInShader(BuiltInShaderType type, OpenGLRenderSystem& renderSystem);

    static std::string GetNameForType(BuiltInShaderType type);
};

}
