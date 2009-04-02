#include "CameraCubeMapDecl.h"
#include "textures/CubeMapTexture.h"
#include "textures/ImageFileLoader.h"

#include <stdexcept>
#include <iostream>

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

// Load an image for the given direction
ImagePtr CameraCubeMapDecl::loadDirection(const std::string& dir) const
{
    ImagePtr img = ImageFileLoader::imageFromVFS(_prefix + dir);
    if (!img)
    {
        throw std::runtime_error(
            "Camera cube map directional image not found: "
            + dir
        );
    }
    return img;
}

/* NamedBindable implementation */

std::string CameraCubeMapDecl::getIdentifier() const
{
    return "_cameraCubeMap_" + _prefix;
}

TexturePtr CameraCubeMapDecl::bindTexture(const std::string& name) const
{
    // Load the six images from the prefix
    try
    {
        ImagePtr left = loadDirection("_left");
        ImagePtr right = loadDirection("_right");
        ImagePtr up = loadDirection("_up");
        ImagePtr down = loadDirection("_down");
        ImagePtr forward = loadDirection("_forward");
        ImagePtr back = loadDirection("_back");
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "[shaders] Unable to bind camera cubemap '"
                  << name << "': " << e.what() << std::endl;
    }

    return TexturePtr(new CubeMapTexture(0, name));
}

}
