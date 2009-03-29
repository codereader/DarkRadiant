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

TexturePtr GLTextureManager::getBinding(MapExpressionPtr mapExp) 
{
	// Check if we got an empty MapExpression, and return the NOT FOUND texture
    // if so
	if (!mapExp)
    {
        return getShaderNotFound();
    }

    // Check if we already have the texture, otherwise construct it
    std::string identifier = mapExp->getIdentifier();
    TextureMap::iterator i = _textures.find(identifier);
    if (i != _textures.end()) 
    {
        // Found, return
        return i->second;
    }
    else
    {
        // This may produce a NULL image if a file can't be found, for example.
        ImagePtr img = mapExp->getImage();

        // see if the MapExpression returned a valid image
        if (img != NULL) 
        {
            // Constructor returned a valid image, now create the texture object
            // and insert this into the map
            BasicTexture2DPtr texture(new BasicTexture2D(identifier));
            _textures.insert(TextureMap::value_type(identifier, texture));
            
            // Bind the texture and get the OpenGL id
            textureFromImage(texture, img);

            globalOutputStream() << "[shaders] Loaded texture: " << identifier << "\n";

            return texture;
        }
        else {
            globalErrorStream() << "[shaders] Unable to load texture: " << identifier << "\n";
            // invalid image produced, return shader not found
            return getShaderNotFound();
        }
    }
}

Texture2DPtr GLTextureManager::getBinding(const std::string& fullPath,
                                          const std::string& moduleNames) 
{
    // check if the texture has to be loaded
    TextureMap::iterator i = _textures.find(fullPath);

	if (i == _textures.end()) {
	    TextureConstructorPtr constructor(new FileLoader(fullPath, moduleNames));
	    ImagePtr img = constructor->construct();

	    // see if the MapExpression returned a valid image
	    if (img != NULL) {
			// Constructor returned a valid image, now create the texture object
            BasicTexture2DPtr basicTex(new BasicTexture2D(fullPath));
			_textures[fullPath] = basicTex;
	
			// Bind the texture and get the OpenGL id
			textureFromImage(basicTex, img);
	
			globalOutputStream() << "[shaders] Loaded texture: " << fullPath << "\n";
	    }
	    else {
	    	globalErrorStream() << "[shaders] Unable to load texture: " << fullPath << "\n";
			// invalid image produced, return shader not found
			return getShaderNotFound();
	    }
	}

    // Cast should succeed since all single image textures will be Texture2D
    return boost::dynamic_pointer_cast<Texture2D>(_textures[fullPath]);
}

// Return the shader-not-found texture, loading if necessary
Texture2DPtr GLTextureManager::getShaderNotFound() 
{
	// Construct the texture if necessary
	if (!_shaderNotFound) {
		_shaderNotFound = loadStandardTexture(SHADER_NOT_FOUND);
	}

	// Return the texture
	return _shaderNotFound;				  
}

Texture2DPtr GLTextureManager::loadStandardTexture(const std::string& filename) 
{
	// Create the texture path
	std::string fullpath = GlobalRegistry().get("user/paths/bitmapsPath") + filename;
	
	BasicTexture2DPtr returnValue(new BasicTexture2D(fullpath));
	
	// load the image with the FileLoader (which can handle .bmp in contrast to the DefaultConstructor)
	TextureConstructorPtr constructor(new FileLoader(fullpath, "bmp"));
	ImagePtr img = constructor->construct();
	
	if (img != ImagePtr()) {
		// Bind the (processed) texture and get the OpenGL id
		// The getProcessed() call may substitute the passed image by another
		textureFromImage(returnValue, img);
	}
	else {
		globalErrorStream() << "[shaders] Couldn't load Standard Texture texture: " 
							<< filename << "\n";
	}
	
	return returnValue;
}

void GLTextureManager::textureFromImage(BasicTexture2DPtr tex2D, 
                                        ImagePtr image) 
{
	// Download the texture and set the reference number
	tex2D->setGLTexNum(image->bindTexture());

	// Fill the Texture structure with the metadata
	tex2D->setWidth(image->getWidth(0));
	tex2D->setHeight(image->getHeight(0));
}

} // namespace shaders
