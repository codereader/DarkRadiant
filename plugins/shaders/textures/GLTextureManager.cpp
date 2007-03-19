#include "GLTextureManager.h"

#include "iradiant.h"
#include "texturelib.h"
#include "igl.h"
#include "FileLoader.h"

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

TexturePtr GLTextureManager::getBinding(const std::string& textureKey, 
										TextureConstructorPtr constructor) 
{
	assert(constructor);

	TextureMap::iterator i = _textures.find(textureKey);
	
	// Check if the texture has to be loaded
	if (i == _textures.end()) {

		// Retrieve the fabricated image from the TextureConstructor
		Image* image = constructor->construct();
		
		if (image != NULL) {

			// Constructor returned a valid image, now create the texture object
			_textures[textureKey] = TexturePtr(new Texture(textureKey));
			
			// Tell the manipulator to do the standard operations 
			// This might return a different image than the passed one
			
			// Do not touch overlay images (no gamma or scaling)
//				if (textureType != texOverlay) {
//					image = _manipulator.getProcessedImage(image);
//				}
			
			// Bind the texture and get the OpenGL id
			load(_textures[textureKey], image);

			// We don't need the image pixel data anymore
			image->release();

			globalOutputStream() << "[shaders] Loaded texture: " 
								 << textureKey.c_str() << "\n";
		}
		else {
			// No image has been loaded, assign it to the "image missing texture"
			globalErrorStream() << "[shaders] Unable to load shader texture: " 
					  			<< textureKey.c_str() << "\n";			
			
			_textures[textureKey] = getShaderNotFound();
		}
	}

	return _textures[textureKey];
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
	// Create the texture constructor
	std::string fullpath = GlobalRegistry().get("user/paths/bitmapsPath") + filename;
	TextureConstructorPtr constructor(new FileLoader(fullpath, "bmp"));
	
	TexturePtr returnValue(new Texture(fullpath));
	
	// Retrieve the fabricated image from the TextureConstructor
	Image* image = constructor->construct();
	
	if (image != NULL) {
		// Bind the (processed) texture and get the OpenGL id
		// The getProcessed() call may substitute the passed image by another
		load(returnValue, image);
		
		// We don't need the (substituted) image pixel data anymore
		image->release();
	}
	else {
		globalErrorStream() << "[shaders] Couldn't load Standard Texture texture: " 
							<< filename.c_str() << "\n";
	}
	
	return returnValue;
}

void GLTextureManager::load(TexturePtr texture, Image* image) {
	
	// Fill the Texture structure with the metadata
	texture->width = image->getWidth();
	texture->height = image->getHeight();
	
	texture->surfaceFlags = image->getSurfaceFlags();
	texture->contentFlags = image->getContentFlags();
	texture->value = image->getValue();
	
	// Calculate an average, representative colour for flatshade rendering 
	texture->color = _manipulator.getFlatshadeColour(image);
	
	// Allocate a new texture number and store it into the Texture structure
	glGenTextures(1, &texture->texture_number);
	glBindTexture(GL_TEXTURE_2D, texture->texture_number);

	// Tell OpenGL how to use the mip maps we will be creating here
	_manipulator.setTextureParameters();
	
	// Now create the mipmaps; conveniently, there exists an openGL method for this 
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, image->getWidth(), image->getHeight(), 
					  GL_RGBA, GL_UNSIGNED_BYTE, image->getRGBAPixels());
	
	// Clear the texture binding
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace shaders
