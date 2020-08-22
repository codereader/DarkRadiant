#pragma once

#include "igl.h"
#include "imodule.h"

typedef unsigned char byte;

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

/**
 * \brief
 * Interface for GL bindable texture objects.
 */
class BindableTexture
{
public:
    virtual ~BindableTexture() {}

    /**
     * \brief
     * Bind this texture to OpenGL.
     *
     * This method invokes the necessary GL calls to bind and upload the
     * object's texture. It returns a Texture object representing the texture in
     * GL.
     *
     * \param name
     * Optional name to give the texture at bind time. This would usually be the
     * name of the image map, e.g. "textures/blah/bleh".
     */
    virtual TexturePtr bindTexture(const std::string& name = "") const = 0;
};

typedef std::shared_ptr<BindableTexture> BindableTexturePtr;

class Image
: public BindableTexture
{
public:

	/**
	 * greebo: Returns the specified mipmap pixel data.
	 */
	virtual byte* getMipMapPixels(std::size_t mipMapIndex) const = 0;

	/**
	 * greebo: Returns the dimension of the specified mipmap.
	 */
	virtual std::size_t getWidth(std::size_t mipMapIndex) const = 0;
	virtual std::size_t getHeight(std::size_t mipMapIndex) const = 0;

	// greebo: Returns TRUE whether this image is precompressed (DDS)
	virtual bool isPrecompressed() const {
		return false;
	}
};
typedef std::shared_ptr<Image> ImagePtr;

class ArchiveFile;

/// Module responsible for loading images from VFS or disk filesystem
class IImageLoader :
	public RegisterableModule
{
public:

    /**
     * \brief
     * Load an image from a VFS path.
     *
     * Images loaded from the VFS are not specified with an extension. Various
     * extensions (and optional VFS directory prefixes) will be attempted in an
     * order specified by the .game file. For example, requesting
     * "textures/blah/bleh" might result in an attempt to load
     * "textures/blah/bleh.tga" followed by "dds/textures/blah/bleh.dds" if the
     * tga version cannot be found.
     */
    virtual ImagePtr imageFromVFS(const std::string& vfsPath) const = 0;

	/**
     * \brief
     * Load an image from a filesystem path.
     */
	virtual ImagePtr imageFromFile(const std::string& filename) const = 0;
};

const char* const MODULE_IMAGELOADER("ImageLoader");

inline const IImageLoader& GlobalImageLoader()
{
    static IImageLoader& _imageLoader(
        *std::static_pointer_cast<IImageLoader>(
            module::GlobalModuleRegistry().getModule(MODULE_IMAGELOADER)
        )
    );

    return _imageLoader;
}
