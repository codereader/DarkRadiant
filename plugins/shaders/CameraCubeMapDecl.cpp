#include "CameraCubeMapDecl.h"
#include "textures/CubeMapTexture.h"

namespace shaders
{

// Constructor
CameraCubeMapDecl::CameraCubeMapDecl(const std::string& prefix)
: _prefix(prefix)
{ }

// Public construction method
NamedBindablePtr CameraCubeMapDecl::createForPrefix(const std::string& prefix)
{
    return NamedBindablePtr(new CameraCubeMapDecl(prefix));
}

/* NamedBindable implementation */

std::string CameraCubeMapDecl::getIdentifier() const
{
    return "_cameraCubeMap_" + _prefix;
}

TexturePtr CameraCubeMapDecl::bindTexture(const std::string& name) const
{
    /* Put GL binding code here */

    return TexturePtr(new CubeMapTexture(0, name));
}

}
