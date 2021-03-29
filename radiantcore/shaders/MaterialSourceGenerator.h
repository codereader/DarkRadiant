#pragma once

#include <string>
#include "ishaders.h"

namespace shaders
{

class ShaderTemplate;

class MaterialSourceGenerator
{
public:
    // Generates the material block contents from the given template
    // (the part within the curly braces, but without the braces themselves)
    static std::string GenerateDefinitionBlock(ShaderTemplate& shaderTemplate);

    // Writes a comment about this material having been created by DarkRadiant
    static void WriteMaterialGenerationComment(std::ostream& stream);

    // Write the name, opening curly braces, block contents, closing curly braces to the given stream
    static void WriteFullMaterialToStream(std::ostream& stream, const MaterialPtr& material);
};

}
