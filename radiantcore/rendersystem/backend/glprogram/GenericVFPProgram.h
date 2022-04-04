#pragma once

#include "iglprogram.h"
#include "igl.h"

namespace render
{

/**
 * An ARB vertex/fragment program as referenced by a idTech4 material 
 * declaration. The programs are loaded from the game's glprogs/ folder.
 */
class GenericVFPProgram : 
    public GLProgram
{
    // openGL Vertex and fragment program identifiers
    GLuint _vertexProgram;
    GLuint _fragmentProgram;

public:
    GenericVFPProgram(const std::string& vertexProgramFilename,
                      const std::string& fragmentProgramFilename);

    // GLProgram implementation
    void create() override;
    void destroy() override;
    void enable() override;
    void disable() override;
};

} // namespace render
