#include "GLTextureManager.h"

#include "qerplugin.h"
#include "texturelib.h"
#include "igl.h"
#include "FileLoader.h"

namespace {
	const int MAX_TEXTURE_QUALITY = 3;
	
	// TODO: Move this into doom3.game or other xml files
	const std::string SHADER_IMAGE_MISSING = "bitmaps/shadernotex.bmp";
	const std::string SHADER_NOT_FOUND = "bitmaps/notex.bmp";
	const std::string SHADER_BUMP_EMPTY = "bitmaps/_flat.bmp";
	const std::string SHADER_SPECULAR_EMPTY = "bitmaps/_black.bmp";
	const std::string SHADER_FALLOFF_EMPTY = "bitmaps/_black.bmp";
}

namespace shaders {

TexturePtr GLTextureManager::getStandardTexture(eTextureType textureType) {
	switch (textureType) {
		case texEditor:
		case texDiffuse:
			return getShaderImageMissing();
		case texBump:
			return getEmptyBump();
		case texSpecular:
			return getEmptySpecular();
		case texLightFalloff:
			return getEmptyFalloff();
		case texOverlay:
			return getShaderImageMissing();
	};
	// This won't be executed, it's for avoiding compiler warnings.
	return getShaderImageMissing();
}

void GLTextureManager::checkBindings() {
	
	// Check the TextureMap for unique pointers and release them as they aren't 
	// used by anyone else than this class.
	for (TextureMap::iterator i = _textures.begin(); 
		 i != _textures.end();
		 ++i) 
	{
		// If the boost::shared_ptr is unique (i.e. refcount==1), remove it.
		// This is explicitly allowed by the std::map iterator invalidation
		// semantics, which specify that erasing an element from a map does not
		// invalidate any other iterators.
		if (i->second.unique()) {
			_textures.erase(i);
		}
	}
}

TexturePtr GLTextureManager::getBinding(const std::string& textureKey, 
										TextureConstructorPtr constructor,
										eTextureType textureType) 
{
	if (textureKey == "") {
		return getStandardTexture(textureType);
	}
	
	TextureMap::iterator i = _textures.find(textureKey);
	
	// Check if the texture has to be loaded
	if (i == _textures.end()) {
		// Is the constructor pointer valid?
		if (constructor != NULL) {
			
			// Retrieve the fabricated image from the TextureConstructor
			Image* image = constructor->construct();
			
			if (image != NULL) {
				// Constructor returned a valid image, now create the texture object
				_textures[textureKey] = TexturePtr(new Texture(textureKey));
				
				// Tell the manipulator to do the standard operations 
				// This might return a different image than the passed one
				
				// Do not touch overlay images (no gamma or scaling)
				if (textureType != texOverlay) {
					image = _manipulator.getProcessedImage(image);
				}
				
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
				
				_textures[textureKey] = getStandardTexture(textureType);
			}
		}
		else {
			globalErrorStream() << "Can't construct texture, constructor is invalid.\n";
						
			// assign this name to the shader image missing texture;
			_textures[textureKey] = getShaderImageMissing();
		}
	}

	return _textures[textureKey];
}

TexturePtr GLTextureManager::loadStandardTexture(const std::string& filename) {
	// Create the texture constructor
	std::string fullpath = GlobalRadiant().getAppPath() + filename;
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

TexturePtr GLTextureManager::getEmptyBump() {
	if (_emptyBump == NULL) {
		_emptyBump = loadStandardTexture(SHADER_BUMP_EMPTY); 
	}
	return _emptyBump;
}

TexturePtr GLTextureManager::getEmptySpecular() {
	if (_emptySpecular == NULL) {
		_emptySpecular = loadStandardTexture(SHADER_SPECULAR_EMPTY); 
	}
	return _emptySpecular;
}

TexturePtr GLTextureManager::getEmptyFalloff() {
	if (_emptyFalloff == NULL) {
		_emptyFalloff = loadStandardTexture(SHADER_FALLOFF_EMPTY); 
	}
	return _emptyFalloff;
}

TexturePtr GLTextureManager::getShaderNotFound() {
	if (_shaderNotFound == NULL) {
		_shaderNotFound = loadStandardTexture(SHADER_NOT_FOUND); 
	}
	return _shaderNotFound;
}

TexturePtr GLTextureManager::getShaderImageMissing() {
	if (_shaderImageMissing == NULL) {
		_shaderImageMissing = loadStandardTexture(SHADER_IMAGE_MISSING); 
	}
	return _shaderImageMissing;
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
