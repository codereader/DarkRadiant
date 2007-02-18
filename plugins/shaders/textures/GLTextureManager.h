#ifndef GLTEXTUREMANAGER_H_
#define GLTEXTUREMANAGER_H_

#include "ishaders.h"
#include <map>
#include "TextureManipulator.h"

namespace shaders {

class GLTextureManager :
	public IGLTextureManager
{
	// The mapping between texturekeys and Texture instances
	typedef std::map<std::string, TexturePtr> TextureMap;
	TextureMap _textures;
	
	TextureManipulator _manipulator;
	
	// The fallback textures in case a texture is empty or broken
	TexturePtr _shaderNotFound;

private:

	/* greebo: Binds the specified texture to openGL and populates the texture object 
	 */
	void load(TexturePtr texture, Image* image);
	
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
	TexturePtr getBinding(const std::string& textureKey, 
						  TextureConstructorPtr constructor);

	/* greebo: This is some sort of "cleanup" call, which causes
	 * the TextureManager to go through the list of textures and 
	 * remove the unused ones. 
	 */ 
	void checkBindings();

};

typedef boost::shared_ptr<GLTextureManager> GLTextureManagerPtr;

} // namespace shaders

#endif /*GLTEXTUREMANAGER_H_*/
