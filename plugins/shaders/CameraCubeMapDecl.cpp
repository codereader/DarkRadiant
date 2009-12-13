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

// Bind directional image
void CameraCubeMapDecl::bindDirection(const std::string& dir,
                                      GLuint glDir) const
{
    // Load the image
    ImagePtr img = ImageFileLoader::imageFromVFS(_prefix + dir);
    if (!img)
    {
        throw std::runtime_error(
            "Camera cube map directional image not found: "
            + _prefix + dir
        );
    }

    // Bind the image to OpenGL
    glTexImage2D(
        glDir,
        0,                    //level
        GL_RGBA,              //internal format
        static_cast<GLsizei>(img->getWidth(0)),   //width
        static_cast<GLsizei>(img->getHeight(0)),  //height
        0,                    //border
        GL_RGBA,               //format
        GL_UNSIGNED_BYTE,     //type
        img->getMipMapPixels(0)
    );
    GlobalOpenGL_debugAssertNoErrors();
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
        // Allocate the GL texture
        GLuint texnum;
        glGenTextures(1, &texnum);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texnum);

        // Set filtering and mipmapping
		glTexParameteri(
            GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
        );
		glTexParameteri(
            GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR
        );
		glTexParameteri(
            GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE
        );

        // Bind the images
        bindDirection("_right", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
        bindDirection("_left", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
        bindDirection("_up", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
        bindDirection("_down", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
        bindDirection("_forward", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
        bindDirection("_back", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

        std::cout << "[shaders] bound cubemap texture " << texnum << std::endl;

        // Unbind and create texture object
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        return TexturePtr(new CubeMapTexture(texnum, name));
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "[shaders] Unable to bind camera cubemap '"
                  << name << "': " << e.what() << std::endl;

        return TexturePtr();
    }
}

}
