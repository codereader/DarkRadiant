#pragma once

#include <string>

namespace shaders
{

class ShaderTemplate;

class MaterialSourceGenerator
{
public:
    // Generates the material block contents from the given template
    // (the part within the curly braces, but without the braces themselves)
    static std::string GenerateDefinitionBlock(ShaderTemplate& shaderTemplate);
};

}
