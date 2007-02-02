#include "GLTextureManager.h"

#include "texturelib.h"

GLTextureManager::GLTextureManager() 
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

TexturePtr GLTextureManager::getBinding(const std::string& textureKey, 
										TextureConstructorPtr constructor) 
{
	iterator i = find(textureKey);
	
	// Check if the texture already exists
	if (i != end()) {
		// It exists
		return _textures[textureKey];
	}
	else {
		// Doesn't exist yet, create a new texture object
		TexturePtr newTexture(new Texture(textureKey));
		_textures[textureKey] = newTexture;
		
		if (constructor != NULL) {
			// Retrieve the fabricated image from the TextureConstructor
			Image* image = constructor->construct();
			
			// Bind the texture and get the OpenGL id
			load(_textures[textureKey], image);
			
			// We don't need the image anymore
			image->release();
		}
		else {
			std::cout << "Can't construct texture, constructor is invalid.\n";
		}
		
		return _textures[textureKey];
	}
}

void GLTextureManager::load(TexturePtr texture, Image* image) {
//void loadTextureRGBA(Texture* q, unsigned char* pPixels, int nWidth, int nHeight) {
		// The gamma value is -1 at radiant startup and gets changed later on
/*		static float fGamma = -1;
		
		float total[3];
		byte *outpixels = 0;
		int nCount = nWidth * nHeight;
	
		if (fGamma != _textureGamma) {
			fGamma = _textureGamma;
			resampleGamma(fGamma);
		}
	
		q->width = nWidth;
		q->height = nHeight;
	
		total[0] = total[1] = total[2] = 0.0f;
	
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
		q->color[2] = total[2] / (nCount * 255);
	
		// Allocate a new texture number and store it into the Texture structure
		glGenTextures(1, &q->texture_number);
	
		glBindTexture(GL_TEXTURE_2D, q->texture_number);
	
		setTextureParameters();
	
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
	
		int quality_reduction = MAX_TEXTURE_QUALITY - _textureQuality;
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
		glTexImage2D(GL_TEXTURE_2D, mip++, _textureComponents, gl_width, gl_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);
		while (gl_width > 1 || gl_height > 1) {
			_manipulator.mipReduce(outpixels, outpixels, gl_width, gl_height, 1, 1);
	
			if (gl_width > 1)
				gl_width >>= 1;
			if (gl_height > 1)
				gl_height >>= 1;
	
			glTexImage2D(GL_TEXTURE_2D, mip++, _textureComponents, gl_width, gl_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);
		}
	
		glBindTexture(GL_TEXTURE_2D, 0);
		if (resampled)
			free(outpixels);*/
}
