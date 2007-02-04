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
	typedef TextureMap::iterator iterator; 
	
	TextureMap _textures;
	
	TextureManipulator _manipulator;

public:
	// Constructor
	GLTextureManager();
	
	// Destructor
	~GLTextureManager();

	iterator begin();
	iterator end();
	iterator find(const std::string& textureKey);

	TexturePtr getBinding(const std::string& textureKey, 
						  TextureConstructorPtr constructor);

private:

	/* greebo: Binds the specified texture to openGL and populates the texture object 
	 */
	void load(TexturePtr texture, Image* image);
};

typedef boost::shared_ptr<GLTextureManager> GLTextureManagerPtr;

} // namespace shaders

#endif /*GLTEXTUREMANAGER_H_*/
