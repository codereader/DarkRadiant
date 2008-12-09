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
			++i;
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
				// and insert this into the map
				std::pair<TextureMap::iterator, bool> result = _textures.insert(
					TextureMap::value_type(identifier, TexturePtr(new Texture(identifier)))
				);
				
				// Bind the texture and get the OpenGL id
				load(result.first->second, img);

				globalOutputStream() << "[shaders] Loaded texture: " << identifier << "\n";

				return result.first->second;
			}
			else {
				globalErrorStream() << "[shaders] Unable to load texture: " << identifier << "\n";
			    // invalid image produced, return shader not found
			    return getShaderNotFound();
			}
		}

		return i->second;
	}
	// We got an empty MapExpression, so we'll return the "image missing texture"
	// globalErrorStream() << "[shaders] Unable to load shader texture.\n";
	return getShaderNotFound();
}

TexturePtr GLTextureManager::getBinding(const std::string& fullPath, const std::string& moduleNames) {
    // check if the texture has to be loaded
    TextureMap::iterator i = _textures.find(fullPath);

	if (i == _textures.end()) {
	    TextureConstructorPtr constructor(new FileLoader(fullPath, moduleNames));
	    ImagePtr img = constructor->construct();

	    // see if the MapExpression returned a valid image
	    if (img != NULL) {
			// Constructor returned a valid image, now create the texture object
			_textures[fullPath] = TexturePtr(new Texture(fullPath));
	
			// Bind the texture and get the OpenGL id
			load(_textures[fullPath], img);
	
			globalOutputStream() << "[shaders] Loaded texture: " << fullPath << "\n";
	    }
	    else {
	    	globalErrorStream() << "[shaders] Unable to load texture: " << fullPath << "\n";
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
							<< filename << "\n";
	}
	
	return returnValue;
}

void GLTextureManager::load(TexturePtr texture, ImagePtr image) {
	// Download the texture and set the reference number
	texture->texture_number = image->downloadTextureToGL();

	// Fill the Texture structure with the metadata
	texture->width = image->getWidth(0);
	texture->height = image->getHeight(0);

	// Flat-shade colour mode will not be supported in the near future (TODO)
	texture->color = Colour3(0.5, 0.5, 0.5);
	
	/*// Calculate an average, representative colour for flatshade rendering 
	texture->color = TextureManipulator::instance().getFlatshadeColour(image);*/
}

} // namespace shaders
