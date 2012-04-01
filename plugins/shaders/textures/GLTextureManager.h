#ifndef GLTEXTUREMANAGER_H_
#define GLTEXTUREMANAGER_H_

#include "ishaders.h"
#include <map>
#include "../MapExpression.h"
#include "texturelib.h"

namespace shaders
{

class GLTextureManager
{
	// The mapping between texturekeys and Texture instances
	typedef std::map<std::string, TexturePtr> TextureMap;
	TextureMap _textures;

	// The fallback textures in case a texture is empty or broken
	TexturePtr _shaderNotFound;

private:

	// Constructs the fallback textures like "Shader Image Missing"
	TexturePtr loadStandardTexture(const std::string& filename);

public:

    /**
     * \brief
     * Construct a bound texture from a generic named bindable.
     */
	TexturePtr getBinding(NamedBindablePtr bindable);

	/** greebo: This loads a texture directly from the disk using the
	 * 			specified <fullPath>.
	 *
	 * @fullPath: The path to the file (no VFS paths).
	 * @moduleNames: The module names used to invoke the correct imageloader.
	 * 				 This defaults to "BMP".
	 */
	TexturePtr getBinding(const std::string& fullPath,
                          const std::string& moduleNames = "bmp");

	/**
     * \brief
     * Get the "shader not found" texture.
     */
	TexturePtr getShaderNotFound();

	/* greebo: This is some sort of "cleanup" call, which causes
	 * the TextureManager to go through the list of textures and
	 * remove the unused ones.
	 */
	void checkBindings();

};

typedef boost::shared_ptr<GLTextureManager> GLTextureManagerPtr;

} // namespace shaders

#endif /*GLTEXTUREMANAGER_H_*/
