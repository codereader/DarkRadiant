#include "BuiltInShader.h"

#include "string/convert.h"

namespace render
{

BuiltInShader::BuiltInShader(BuiltInShaderType type, OpenGLRenderSystem& renderSystem) :
    OpenGLShader(GetNameForType(type), renderSystem)
{}



std::string BuiltInShader::GetNameForType(BuiltInShaderType type)
{
    return "$BUILT_IN_SHADER[" + string::to_string(static_cast<std::size_t>(type)) + "]";
}

}
