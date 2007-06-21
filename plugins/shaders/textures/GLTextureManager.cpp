#include "GLTextureManager.h"

#include "iradiant.h"
#include "texturelib.h"
#include "igl.h"
#include "FileLoader.h"
#include "../MapExpression.h"
#include "TextureManipulator.h"
#include "parser/DefTokeniser.h"

namespace {
	const int MAX_TEXTURE_QUALITY = 3;
	
	const std::string SHADER_NOT_FOUND = "notex.bmp";
}

namespace shaders {

void GLTextureManager::checkBindings() {
	// Check the TextureMap for unique pointers and release them
	// as they aren't used by anyone else than this class.
	for (TextureMap::iterator i = _textures.begin(); 
		 i != _textures.end(); 
		 /* in-loop increment */) 
	{
		// If the boost::shared_ptr is unique (i.e. refcount==1), remove it
		if (i->second.unique()) {
			// Be sure to increment the iterator with a postfix ++, 
			// so that the iterator is incremented right before deletion
			_textures.erase(i++);
		}
		else {
			i++;
		}
	}
}

TexturePtr GLTextureManager::getBinding(MapExpressionPtr mapExp) {
	// check if we got an empty MapExpression
	if (mapExp != NULL) {
		// check if the texture has to be loaded
		std::string identifier = mapExp->getIdentifier();
		TextureMap::iterator i = _textures.find(identifier);
		if (i == _textures.end()) {
			// This may produce a NULL image if a file can't be found, for example.
			ImagePtr img = mapExp->getImage();

			// see if the MapExpression returned a valid image
			if (img != NULL) {
				// Constructor returned a valid image, now create the texture object
				_textures[identifier] = TexturePtr(new Texture(identifier));
			
				// Bind the texture and get the OpenGL id
				load(_textures[identifier], img);

				globalOutputStream() << "[shaders] Loaded texture: " << identifier.c_str() << "\n";
			}
			else {
			    // invalid image produced, return shader not found
			    return getShaderNotFound();
			}
		}
		return _textures[identifier];
	}
	// We got an empty MapExpression, so we'll return the "image missing texture"
	globalErrorStream() << "[shaders] Unable to load shader texture";
	return getShaderNotFound();
}

TexturePtr GLTextureManager::getBinding(const std::string& fullPath, const std::string& moduleNames) {
    // check if the texture has to be loaded
    TextureMap::iterator i = _textures.find(fullPath);

	if (i == _textures.end()) {
	    TextureConstructorPtr constructor(new FileLoader(fullPath, moduleNames));
	    ImagePtr img = constructor->construct();

	    // see if the MapExpression returned a valid image
	    if (img != ImagePtr()) {
			// Constructor returned a valid image, now create the texture object
			_textures[fullPath] = TexturePtr(new Texture(fullPath));
	
			// Bind the texture and get the OpenGL id
			load(_textures[fullPath], img);
	
			globalOutputStream() << "[shaders] Loaded texture: " << fullPath.c_str() << "\n";
	    }
	    else {
	    	globalErrorStream() << "[shaders] Unable to load texture: " << fullPath.c_str() << "\n";
			// invalid image produced, return shader not found
			return getShaderNotFound();
	    }
	}
    return _textures[fullPath];
}

// Return the shader-not-found texture, loading if necessary
TexturePtr GLTextureManager::getShaderNotFound() {

	// Construct the texture if necessary
	if (!_shaderNotFound) {
		_shaderNotFound = loadStandardTexture(SHADER_NOT_FOUND);
	}

	// Return the texture
	return _shaderNotFound;				  
}

TexturePtr GLTextureManager::loadStandardTexture(const std::string& filename) {
	// Create the texture path
	std::string fullpath = GlobalRegistry().get("user/paths/bitmapsPath") + filename;
	
	TexturePtr returnValue(new Texture(fullpath));
	
	// load the image with the FileLoader (which can handle .bmp in contrast to the DefaultConstructor)
	TextureConstructorPtr constructor(new FileLoader(fullpath, "bmp"));
	ImagePtr img = constructor->construct();
	
	if (img != ImagePtr()) {
		// Bind the (processed) texture and get the OpenGL id
		// The getProcessed() call may substitute the passed image by another
		load(returnValue, img);
	}
	else {
		globalErrorStream() << "[shaders] Couldn't load Standard Texture texture: " 
							<< filename.c_str() << "\n";
	}
	
	return returnValue;
}

void GLTextureManager::load(TexturePtr texture, ImagePtr image) {
	
	// Fill the Texture structure with the metadata
	texture->width = image->getWidth();
	texture->height = image->getHeight();
	
	texture->surfaceFlags = image->getSurfaceFlags();
	texture->contentFlags = image->getContentFlags();
	texture->value = image->getValue();
	
	// Calculate an average, representative colour for flatshade rendering 
	texture->color = TextureManipulator::instance().getFlatshadeColour(image);
	
	// Allocate a new texture number and store it into the Texture structure
	glGenTextures(1, &texture->texture_number);
	glBindTexture(GL_TEXTURE_2D, texture->texture_number);

	// Tell OpenGL how to use the mip maps we will be creating here
	TextureManipulator::instance().setTextureParameters();
	
	// Now create the mipmaps; conveniently, there exists an openGL method for this 
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, image->getWidth(), image->getHeight(), 
					  GL_RGBA, GL_UNSIGNED_BYTE, image->getRGBAPixels());
	
	// Clear the texture binding
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace shaders
