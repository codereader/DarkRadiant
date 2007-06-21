#ifndef GLTEXTUREMANAGER_H_
#define GLTEXTUREMANAGER_H_

#include "ishaders.h"
#include <map>
#include "../MapExpression.h"

namespace shaders {

class GLTextureManager :
	public IGLTextureManager
{
	// The mapping between texturekeys and Texture instances
	typedef std::map<std::string, TexturePtr> TextureMap;
	TextureMap _textures;
	
	// The fallback textures in case a texture is empty or broken
	TexturePtr _shaderNotFound;

private:

	/* greebo: Binds the specified texture to openGL and populates the texture object 
	 */
	void load(TexturePtr texture, ImagePtr image);
	
	// Constructs the fallback textures like "Shader Image Missing"
	TexturePtr loadStandardTexture(const std::string& filename);

	// Construct and return the "shader not found" texture
	TexturePtr getShaderNotFound();

public:

	/* greebo: Use this method to request a Texture to be realised
	 * (i.e. loaded into graphics memory and assigned a texture_number).
	 * 
	 * This will return the realised texture or a "fallback" texture
	 * according to the given <textureType> (flat image for normalmaps,
	 * black for specular), if the texture is empty or can't be loaded.  
	 */
	TexturePtr getBinding(MapExpressionPtr mapExp);
	
	/** greebo: This loads a texture directly from the disk using the
	 * 			specified <fullPath>.
	 * 
	 * @fullPath: The path to the file (no VFS paths).
	 * @moduleNames: The module names used to invoke the correct imageloader.
	 * 				 This defaults to "BMP".
	 */
	TexturePtr getBinding(const std::string& fullPath,
				 		const std::string& moduleNames = "bmp");

	/* greebo: This is some sort of "cleanup" call, which causes
	 * the TextureManager to go through the list of textures and 
	 * remove the unused ones. 
	 */ 
	void checkBindings();

};

typedef boost::shared_ptr<GLTextureManager> GLTextureManagerPtr;

} // namespace shaders

#endif /*GLTEXTUREMANAGER_H_*/
