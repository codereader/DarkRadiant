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

GLTextureManager::GLTextureManager() 
{}

GLTextureManager::~GLTextureManager() 
{}

GLTextureManager::iterator GLTextureManager::begin() {
	return _textures.begin();
}

GLTextureManager::iterator GLTextureManager::end() {
	return _textures.end();
}

GLTextureManager::iterator GLTextureManager::find(const std::string& textureKey) {
	return _textures.find(textureKey);
}

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
	};
	// This won't be executed, it's for avoiding compiler warnings.
	return getShaderImageMissing();
}

void GLTextureManager::checkBindings() {
	// Check the TextureMap for unique pointers and release them
	// as they aren't used by anyone else than this class.
	for (iterator i = begin(); i != end(); ) {
		// If the boost::shared_ptr is unique (i.e. refcount==1), remove it
		if (i->second.unique()) {
			// Be sure to increment the iterator with a postfix ++, 
			// so that the "old" iterator is passed
			_textures.erase(i++);
		}
		else {
			i++;
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
	
	iterator i = find(textureKey);
	
	// Check if the texture has to be loaded
	if (i == end()) {
		// Is the constructor pointer valid?
		if (constructor != NULL) {
			// Retrieve the fabricated image from the TextureConstructor
			Image* image = constructor->construct();
			
			if (image != NULL) {
				// Constructor returned a valid image, now create the texture object
				_textures[textureKey] = TexturePtr(new Texture(textureKey));
				
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
		// Bind the texture and get the OpenGL id
		load(returnValue, image);
		
		// We don't need the image pixel data anymore
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
	
	int nWidth = image->getWidth();
	int nHeight = image->getHeight();
	
	texture->width = nWidth;
	texture->height = nHeight;
	
	texture->surfaceFlags = image->getSurfaceFlags();
	texture->contentFlags = image->getContentFlags();
	texture->value = image->getValue();
	
	unsigned char* pPixels = image->getRGBAPixels();
	
	byte* outpixels;
	
/*	total[0] = total[1] = total[2] = 0.0f;
	
		// resample texture gamma according to user settings
		for (int i = 0; i < (nCount * 4); i += 4) {
			for (int j = 0; j < 3; j++) {
				total[j] += (pPixels + i)[j];
				byte b = (pPixels + i)[j];
				(pPixels + i)[j] = _gammaTable[b];
			}
		}
	
		q->color[0] = total[0] / (nCount * 255);
		q->color[1] = total[1] / (nCount * 255);
		q->color[2] = total[2] / (nCount * 255);*/
	
	// Allocate a new texture number and store it into the Texture structure
	glGenTextures(1, &texture->texture_number);
	glBindTexture(GL_TEXTURE_2D, texture->texture_number);

	//setTextureParameters();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
	int gl_width = 1;
	while (gl_width < nWidth)
		gl_width <<= 1;
	
	int gl_height = 1;
	while (gl_height < nHeight)
		gl_height <<= 1;
	
	bool resampled = false;
		
	if (!(gl_width == nWidth && gl_height == nHeight)) {
		resampled = true;
		outpixels = (byte *)malloc(gl_width * gl_height * 4);
		_manipulator.resampleTexture(pPixels, nWidth, nHeight, outpixels, gl_width, gl_height, 4);
	}
	else {
		outpixels = pPixels;
	}
	
	int _maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_maxTextureSize);
	if (_maxTextureSize == 0) {
		_maxTextureSize = 1024;
	}
	
	int quality_reduction = MAX_TEXTURE_QUALITY - MAX_TEXTURE_QUALITY;//_textureQuality;
	int target_width = std::min(gl_width >> quality_reduction, _maxTextureSize);
	int target_height = std::min(gl_height >> quality_reduction, _maxTextureSize);
	
	while (gl_width > target_width || gl_height > target_height) {
		_manipulator.mipReduce(outpixels, outpixels, gl_width, gl_height, target_width, target_height);

		if (gl_width > target_width)
			gl_width >>= 1;
		if (gl_height > target_height)
			gl_height >>= 1;
	}

	int mip = 0;
	glTexImage2D(GL_TEXTURE_2D, mip++, GL_RGBA, gl_width, gl_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);
	while (gl_width > 1 || gl_height > 1) {
		_manipulator.mipReduce(outpixels, outpixels, gl_width, gl_height, 1, 1);

		if (gl_width > 1)
			gl_width >>= 1;
		if (gl_height > 1)
			gl_height >>= 1;

		glTexImage2D(GL_TEXTURE_2D, mip++, GL_RGBA, gl_width, gl_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	if (resampled)
		free(outpixels);
}

} // namespace shaders
