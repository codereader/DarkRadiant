#pragma once

#include "ifilesystem.h"

#include <map>
#include <string>
#include "ShaderTemplate.h"
#include "ShaderNameCompareFunctor.h"

namespace shaders
{

/**
 * Wrapper class that associates a ShaderTemplate with its filename.
 */
struct ShaderDefinition
{
    // The shader template
    ShaderTemplatePtr shaderTemplate;

    // File from which the shader was parsed
    vfs::FileInfo file;

    /* Constructor
     */
    explicit ShaderDefinition(const ShaderTemplatePtr& templ,
                              const vfs::FileInfo& f = vfs::FileInfo()):
        shaderTemplate(templ),
        file(f)
    {}

};

typedef std::map<std::string, ShaderDefinition, ShaderNameCompareFunctor> ShaderDefinitionMap;

}
