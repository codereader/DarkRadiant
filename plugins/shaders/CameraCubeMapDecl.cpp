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

// Bind directional image
void CameraCubeMapDecl::bindDirection(GLuint glDir, ImagePtr img) const
{
    glTexImage2D(
        glDir,
        0,                    //level
        GL_RGB8,              //internal format
        img->getWidth(0),   //width
        img->getHeight(0),  //height
        0,                    //border
        GL_RGB,               //format
        GL_UNSIGNED_BYTE,     //type
        img->getMipMapPixels(0)
    );
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

        // Allocate the GL texture
        GLuint texnum;
        glGenTextures(1, &texnum);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texnum);

        // Bind the images
        bindDirection(GL_TEXTURE_CUBE_MAP_POSITIVE_X, right);
        bindDirection(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left);
        bindDirection(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, up);
        bindDirection(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, down);
        bindDirection(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back);
        bindDirection(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, forward);

        std::cout << "[shaders] bound cubemap texture " << texnum << std::endl;

        // Unbind and create texture object
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        return TexturePtr(new CubeMapTexture(0, name));
        //return TexturePtr(new CubeMapTexture(texnum, name));
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "[shaders] Unable to bind camera cubemap '"
                  << name << "': " << e.what() << std::endl;

        return TexturePtr();
    }
}

}
