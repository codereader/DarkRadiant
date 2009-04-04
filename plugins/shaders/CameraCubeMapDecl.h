#pragma once

#include "NamedBindable.h"

namespace shaders
{

/**
 * \brief
 * Declaration parsed from a "cameraCubeMap" stage in a material definition.
 *
 * A cameraCubeMap specifies a texture prefix, to which the suffixes "_up",
 * "_down", "_left", "_right", "_forward" and "_back" are appended to give the
 * six cube map images.
 *
 * A CameraCubeMapDecl object stores the texture prefix and implements the
 * NamedBindable interface so that it can bind the cube map texture in GL and
 * return it to the texture manager.
 */
class CameraCubeMapDecl
: public NamedBindable
{
    // The texture prefix
    std::string _prefix;

private:

    // Construct with a prefix
    CameraCubeMapDecl(const std::string& prefix);

    // Load and bind the given image with the given cube-map direction
    void bindDirection(const std::string& dir, GLuint glDir) const;

public:

    /* NamedBindable implementation */
    std::string getIdentifier() const;
    TexturePtr bindTexture(const std::string& name) const;

    /**
     * \brief
     * Construct and return a CameraCubeMapDecl from the given texture prefix.
     */
    static NamedBindablePtr createForPrefix(const std::string& prefix);
};

}

