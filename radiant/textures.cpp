/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "textures.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include "iregistry.h"
#include "qerplugin.h"
#include "itextures.h"
#include "igl.h"
#include "preferencesystem.h"
#include "qgl.h"

#include "texturelib.h"
#include "container/hashfunc.h"
#include "container/cache.h"
#include "stringio.h"

#include "image.h"
#include "texmanip.h"

enum TextureCompressionFormat {
    TEXTURECOMPRESSION_NONE = 0,
    TEXTURECOMPRESSION_RGBA = 1,
    TEXTURECOMPRESSION_RGBA_S3TC_DXT1 = 2,
    TEXTURECOMPRESSION_RGBA_S3TC_DXT3 = 3,
    TEXTURECOMPRESSION_RGBA_S3TC_DXT5 = 4,
};

typedef std::pair<LoadImageCallback, CopiedString> TextureKey;

class TextureKeyEqualNoCase
{
public:
  bool operator()(const TextureKey& key, const TextureKey& other) const
  {
    return key.first == other.first && string_equal_nocase(key.second.c_str(), other.second.c_str());
  }
};

class TextureKeyHashNoCase
{
public:
  typedef hash_t hash_type;
  hash_t operator()(const TextureKey& key) const
  {
    return hash_combine(string_hash_nocase(key.second.c_str()), pod_hash(key.first));
  }
};

#define DEBUG_TEXTURES 0

namespace {
	const int MAX_TEXTURE_QUALITY = 3;
	
	const std::string RKEY_TEXTURES_QUALITY = "user/ui/textures/quality";
	const std::string RKEY_TEXTURES_MODE = "user/ui/textures/mode";
	const std::string RKEY_TEXTURES_GAMMA = "user/ui/textures/gamma";
	const std::string RKEY_TEXTURES_COMPRESSIONFORMAT = "user/ui/textures/compressionFormat";
}

/* greebo: This is the implementation of the ABC TexturesCache defined in itextures.h
 * 
 * It maintains a list of all the loaded textures and the according preferences
 * and provides methods to capture and release textures (from files).
 */
class TexturesMap : 
	public TexturesCache,
	public PreferenceConstructor,
	public RegistryKeyObserver
{
	class TextureConstructor 
	{
		// The linkback to the TexturesMap parent class
		TexturesMap* m_cache;
	public:
	
		// Nonconverting constructor
		explicit TextureConstructor(TexturesMap* cache)
			: m_cache(cache) 
		{}
		
		// Construct a new Texture
		qtexture_t* construct(const TextureKey& key) {
			
			// Allocate a new Texture object with a load callback and a name
			qtexture_t* texture = new qtexture_t(key.first, key.second.c_str());
			
			// If the other texturesMap is already realised, realise this texture as well
			if (m_cache->realised()) {
				m_cache->realiseTexture(*texture, key);
			}
			return texture;
		}
		
		// Releases a texture from the heap
		void destroy(qtexture_t* texture) {
			if (m_cache->realised()) {
				// Only destroy this distinct texture if all the others are still realised
				// otherwise this texture isn't realised any more as well.
				m_cache->unrealiseTexture(*texture);
			}
			
			// Remove it from the heap
			delete texture;
		}
	};
	
	byte _gammaTable[256];

	typedef HashedCache<TextureKey, qtexture_t, TextureKeyHashNoCase, TextureKeyEqualNoCase, TextureConstructor> qtextures_t;
	typedef qtextures_t::iterator iterator;
	
	qtextures_t m_qtextures;
	TexturesCacheObserver* m_observer;
	std::size_t m_unrealised;
	
	ETexturesMode _textureMode;
	
	// The connected callbacks	
	typedef std::list<TextureModeObserver*> TextureModeObserverList;
	
	TextureModeObserverList _textureModeObservers;

	int _textureQuality;
	float _textureGamma;
	
	// Gets initialised by an openGL query
	int _maxTextureSize; 
	
	bool _hardwareTextureCompressionSupported;
	bool _openGLCompressionSupported;
	bool _S3CompressionSupported;
	
	TextureCompressionFormat _textureCompressionFormat;
	
	GLint _textureComponents;

public:
	TexturesMap() : 
		m_qtextures(TextureConstructor(this)), 
		m_observer(0), 
		m_unrealised(1),
		_textureQuality(GlobalRegistry().getInt(RKEY_TEXTURES_QUALITY)),
		_textureGamma(GlobalRegistry().getFloat(RKEY_TEXTURES_GAMMA)),
		_maxTextureSize(0),
		_hardwareTextureCompressionSupported(false),
		_openGLCompressionSupported(false),
		_S3CompressionSupported(false),
		_textureCompressionFormat(TEXTURECOMPRESSION_NONE),
		_textureComponents(GL_RGBA)
	{
		GlobalRegistry().addKeyObserver(this, RKEY_TEXTURES_QUALITY);
		GlobalRegistry().addKeyObserver(this, RKEY_TEXTURES_MODE);
		GlobalRegistry().addKeyObserver(this, RKEY_TEXTURES_GAMMA);
		
		// Load the texture mode
		_textureMode = readTextureMode(GlobalRegistry().getInt(RKEY_TEXTURES_MODE));
		
		// Set the texture compression format according to the saved registry value
		setTextureCompressionFormat(GlobalRegistry().getInt(RKEY_TEXTURES_COMPRESSIONFORMAT));
		
		// greebo: Don't know if it makes sense to put this into the constructor, but I
		// kept it as it was this way in the original GtkRadiant code.
		modeChanged();
		
		// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
		GlobalPreferenceSystem().addConstructor(this);
	}
	
	ETexturesMode readTextureMode(const unsigned int& mode) {
		switch (mode) {
			case 0: return eTextures_NEAREST;
			case 1: return eTextures_NEAREST_MIPMAP_NEAREST;
			case 2: return eTextures_NEAREST_MIPMAP_LINEAR;
    		case 3: return eTextures_LINEAR;
    		case 4: return eTextures_LINEAR_MIPMAP_NEAREST;
    		case 5: return eTextures_LINEAR_MIPMAP_LINEAR;
    		default: return eTextures_NEAREST;
		}
	}
	
	ETexturesMode getTextureMode() const {
		return _textureMode;
	}
	
	void setTextureMode(ETexturesMode mode) {
		if (_textureMode != mode) {
			// keyChanged() is automatically triggered and sets the member _textureMode
			GlobalRegistry().setInt(RKEY_TEXTURES_MODE, mode);
			
			// Take the actions on modeChanged()
			modeChanged();
		}
	}
	
	// RegistryKeyObserver implementation
	void keyChanged() {
		_textureQuality = GlobalRegistry().getInt(RKEY_TEXTURES_QUALITY);
		_textureMode = readTextureMode(GlobalRegistry().getInt(RKEY_TEXTURES_MODE));
		
		float newGamma = GlobalRegistry().getFloat(RKEY_TEXTURES_GAMMA);

		// Has the gamma actually changed? 
		if (newGamma != _textureGamma) {
			unrealise();
			
			_textureGamma = newGamma;
			
			setTextureParameters();
			realise();
		}
		
		setTextureCompressionFormat(GlobalRegistry().getInt(RKEY_TEXTURES_COMPRESSIONFORMAT));
	}
	
	void modeChanged() {
		if (realised()) {
			setTextureParameters();
			for (TexturesMap::iterator i = begin(); i != end(); ++i) {
				glBindTexture (GL_TEXTURE_2D, i->value->texture_number);
				setTextureParameters();
			}
	
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		
		modeChangeNotify();
	}
	
	void setTextureParameters() {
		switch (_textureMode) {
			case eTextures_NEAREST:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				break;
			case eTextures_NEAREST_MIPMAP_NEAREST:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				break;
			case eTextures_NEAREST_MIPMAP_LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				break;
			case eTextures_LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				break;
			case eTextures_LINEAR_MIPMAP_NEAREST:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				break;
			case eTextures_LINEAR_MIPMAP_LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				break;
			default:
				globalOutputStream() << "invalid texture mode\n";
		}
	}
	
	void setTextureComponents(GLint textureComponents) {
		if (_textureComponents != textureComponents) {
			unrealise();
			
			_textureComponents = textureComponents;
			
			setTextureParameters();
			realise();
		}
	}

	iterator begin() {
		return m_qtextures.begin();
	}
	
	iterator end() {
		return m_qtextures.end();
	}

	LoadImageCallback defaultLoader() const {
		// Return the QERApp image loader that cycles through all the image modules
		return LoadImageCallback(NULL, QERApp_LoadImage);
	}
	
	Image* loadImage(const std::string& name) {
		return defaultLoader().loadImage(name);
	}
	
	// Capture the named texture
	qtexture_t* capture(const std::string& name) {
		return capture(defaultLoader(), name);
	}

	// Capture the named texture using the provided image loader
	qtexture_t* capture(const LoadImageCallback& loader,
	                    const std::string& name) {
		return m_qtextures.capture(TextureKey(loader, name.c_str())).get();
	}

	void release(qtexture_t* texture) {
		
		#if DEBUG_TEXTURES
		globalOutputStream() << "textures release: " << makeQuoted(texture->name) << '\n';
		#endif
		m_qtextures.release(TextureKey(texture->load, texture->name.c_str()));
	}
	
	void attach(TexturesCacheObserver& observer) {
		ASSERT_MESSAGE(m_observer == 0, "TexturesMap::attach: cannot attach observer");
		m_observer = &observer;
	}
	
	void detach(TexturesCacheObserver& observer) {
		ASSERT_MESSAGE(m_observer == &observer, "TexturesMap::detach: cannot detach observer");
		m_observer = 0;
	}
	
	/// \brief This function does the actual processing of raw RGBA data into a GL texture.
	/// It will also resample to power-of-two dimensions, generate the mipmaps and adjust gamma.
	void loadTextureRGBA(qtexture_t* q, unsigned char* pPixels, int nWidth, int nHeight) {
		
		// The gamma value is -1 at radiant startup and gets changed later on
		static float fGamma = -1;
		
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
	
		GlobalTexturesCache().setTextureParameters();
	
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
			R_ResampleTexture(pPixels, nWidth, nHeight, outpixels, gl_width, gl_height, 4);
		}
		else {
			outpixels = pPixels;
		}
	
		int quality_reduction = MAX_TEXTURE_QUALITY - _textureQuality;
		int target_width = std::min(gl_width >> quality_reduction, _maxTextureSize);
		int target_height = std::min(gl_height >> quality_reduction, _maxTextureSize);
	
		while (gl_width > target_width || gl_height > target_height) {
			GL_MipReduce(outpixels, outpixels, gl_width, gl_height, target_width, target_height);
	
			if (gl_width > target_width)
				gl_width >>= 1;
			if (gl_height > target_height)
				gl_height >>= 1;
		}
	
		int mip = 0;
		glTexImage2D(GL_TEXTURE_2D, mip++, _textureComponents, gl_width, gl_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);
		while (gl_width > 1 || gl_height > 1) {
			GL_MipReduce(outpixels, outpixels, gl_width, gl_height, 1, 1);
	
			if (gl_width > 1)
				gl_width >>= 1;
			if (gl_height > 1)
				gl_height >>= 1;
	
			glTexImage2D(GL_TEXTURE_2D, mip++, _textureComponents, gl_width, gl_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outpixels);
		}
	
		glBindTexture(GL_TEXTURE_2D, 0);
		if (resampled)
			free(outpixels);
	}	
	
	void realiseTexture(qtexture_t& texture, const TextureKey& key) {

		texture.texture_number = 0;
		
		if (!string_empty(key.second.c_str())) {
			
			// Try to load the image using the specified loader
			// TODO: remove the c_str() as soon as the CopiedString is gone
			Image* image = key.first.loadImage(key.second.c_str());
			
			// If the image load was successful, the pointer is not NULL
			if (image != NULL) {
				
				// Load the actual pixel data
				loadTextureRGBA(&texture, image->getRGBAPixels(), image->getWidth(), image->getHeight());
				
				texture.surfaceFlags = image->getSurfaceFlags();
				texture.contentFlags = image->getContentFlags();
				texture.value = image->getValue();
				
				// Delete the image object (usually from the heap)
				image->release();
				
				globalOutputStream() << "Loaded Texture: \"" << key.second.c_str() << "\"\n";
				GlobalOpenGL_debugAssertNoErrors();
			}
			else {
				globalErrorStream() << "Texture load failed: \"" << key.second.c_str() << "\"\n";
			}
		}
	}
	
	// This deletes a given texture from OpenGL
	void unrealiseTexture(qtexture_t& texture) {
		// Sanity checks
		if (GlobalOpenGL().contextValid && texture.texture_number != 0) {
			glDeleteTextures(1, &texture.texture_number);
			GlobalOpenGL_debugAssertNoErrors();
		}
	}

	
	void realise() {
		if (--m_unrealised == 0) {
			_hardwareTextureCompressionSupported = false;

			if (GLEW_ARB_texture_compression) {
				_hardwareTextureCompressionSupported = true;
				_openGLCompressionSupported = true;
			}

			if (GLEW_EXT_texture_compression_s3tc) {
				_hardwareTextureCompressionSupported = true;
				_S3CompressionSupported = true;
			}

			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_maxTextureSize);
			if (_maxTextureSize == 0) {
				_maxTextureSize = 1024;
			}

			for (qtextures_t::iterator i = m_qtextures.begin(); i != m_qtextures.end(); ++i) {
				if (!i->value.empty()) {
					realiseTexture(*(i->value), i->key);
				}
			}
			if (m_observer != 0) {
				m_observer->realise();
			}
		}
	}
	
	void unrealise() {
		if (++m_unrealised == 1) {
			if (m_observer != 0) {
				m_observer->unrealise();
			}
			
			// Now unrealise all the textures
			for (qtextures_t::iterator i = m_qtextures.begin(); i != m_qtextures.end(); ++i) {
				if (!i->value.empty()) {
					unrealiseTexture(*(i->value));
				}
			}
		}
	}

	bool realised() const {
		return m_unrealised == 0;
	}
	
	void addTextureModeObserver(TextureModeObserver* observer) {
		if (observer != NULL) {
			// Add the passed observer to the list
			_textureModeObservers.push_back(observer);
		}
	}
	
	void removeTextureModeObserver(TextureModeObserver* observer) {
		// Cycle through the list of observers and call the moved method
		for (TextureModeObserverList::iterator i = _textureModeObservers.begin(); 
			 i != _textureModeObservers.end(); 
			 i++) 
		{
			TextureModeObserver* registered = *i;
			
			if (registered == observer) {
				_textureModeObservers.erase(i++);
				return; // Don't continue the loop, the iterator is obsolete 
			}
		}
	}
	
	// Call the attached observers
	void modeChangeNotify() {
		// Cycle through the list of observers and call the changed() method
		for (TextureModeObserverList::iterator i = _textureModeObservers.begin(); 
			 i != _textureModeObservers.end(); 
			 i++) 
		{
			TextureModeObserver* observer = *i;
			
			if (observer != NULL) {
				observer->textureModeChanged();
			}
		}
	}
	
	/* greebo: This gets called by the preference system and is responsible for adding the
	 * according pages and elements to the preference dialog.*/
	void constructPreferencePage(PreferenceGroup& group) {
		PreferencesPage* page(group.createPage("Textures", "Texture Settings"));
		
		// Create the string list containing the quality captions
		std::list<std::string> percentages;
		
		percentages.push_back("12.5%");
		percentages.push_back("25%");
		percentages.push_back("50%");
		percentages.push_back("100%");
		
		page->appendCombo("Texture Quality", RKEY_TEXTURES_QUALITY, percentages);
		
		// Texture Gamma Settings
		page->appendSpinner("Texture Gamma", RKEY_TEXTURES_GAMMA, 0.0f, 1.0f, 10);
		
		// Create the string list containing the mode captions
		std::list<std::string> textureModes;
		
		textureModes.push_back("Nearest");
		textureModes.push_back("Nearest Mipmap");
		textureModes.push_back("Linear");
		textureModes.push_back("Bilinear");
		textureModes.push_back("Bilinear Mipmap");
		textureModes.push_back("Trilinear");
		
		page->appendCombo("Texture Render Mode", RKEY_TEXTURES_MODE, textureModes);
		
		// Create the string list containing the mode captions
		std::list<std::string> compressionModes;
		
		compressionModes.push_back("None");
		
		if (_openGLCompressionSupported) {
			compressionModes.push_back("OpenGL ARB");
		}
		
		if (_S3CompressionSupported) {
			// OpenGL + S3
			compressionModes.push_back("S3TC DXT1");
			compressionModes.push_back("S3TC DXT3");
			compressionModes.push_back("S3TC DXT5");
		}
		
		page->appendCombo("Hardware Texture Compression", 
						  RKEY_TEXTURES_COMPRESSIONFORMAT, 
						  compressionModes);
	}

	/* greebo: Translate the number from the combo box into a
	 * valid texture compression enum. (Takes the various possible
	 * combinations into account (OpenGL, S3, None)).
	 */
	void setTextureCompressionFormat(unsigned int value) {
	
		if (!_openGLCompressionSupported && _S3CompressionSupported && value >= 1) {
			// This takes the missing "OpenGL ARB" into account if the openGLcompression
			// is disabled and S3compression is enabled (for compression != NONE).
			++value;
		}
		
		switch (value) {
			case 0:
				_textureCompressionFormat = TEXTURECOMPRESSION_NONE;
				break;
			case 1:
				_textureCompressionFormat = TEXTURECOMPRESSION_RGBA;
				break;
			case 2:
				_textureCompressionFormat = TEXTURECOMPRESSION_RGBA_S3TC_DXT1;
				break;
			case 3:
				_textureCompressionFormat = TEXTURECOMPRESSION_RGBA_S3TC_DXT3;
				break;
			case 4:
				_textureCompressionFormat = TEXTURECOMPRESSION_RGBA_S3TC_DXT5;
				break;
		}
		
		updateTextureCompressionFormat();
	}

	void updateTextureCompressionFormat() {
		GLint textureComponents = GL_RGBA;
	
		if (_hardwareTextureCompressionSupported) {
			if (_textureCompressionFormat != TEXTURECOMPRESSION_NONE 
				&& _textureCompressionFormat != TEXTURECOMPRESSION_RGBA
				&& !_S3CompressionSupported) 
			{
				globalOutputStream() << "OpenGL extension GL_EXT_texture_compression_s3tc not supported by current graphics drivers\n";
				_textureCompressionFormat = TEXTURECOMPRESSION_RGBA; // if this is not supported either, see below
			}
			
			if (_textureCompressionFormat == TEXTURECOMPRESSION_RGBA 
				&& !_openGLCompressionSupported) 
			{
				globalOutputStream() << "OpenGL extension GL_ARB_texture_compression not supported by current graphics drivers\n";
				_textureCompressionFormat = TEXTURECOMPRESSION_NONE;
			}
	
			switch (_textureCompressionFormat) {
				case (TEXTURECOMPRESSION_NONE): {
					textureComponents = GL_RGBA;
					break;
				}
				case (TEXTURECOMPRESSION_RGBA): {
					textureComponents = GL_COMPRESSED_RGBA_ARB;
					break;
				}
				case (TEXTURECOMPRESSION_RGBA_S3TC_DXT1): {
					textureComponents = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					break;
				}
				case (TEXTURECOMPRESSION_RGBA_S3TC_DXT3): {
					textureComponents = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					break;
				}
				case (TEXTURECOMPRESSION_RGBA_S3TC_DXT5): {
					textureComponents = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					break;
				}
			}
		}
		else {
			textureComponents = GL_RGBA;
			_textureCompressionFormat = TEXTURECOMPRESSION_NONE;
		}
	
		setTextureComponents(textureComponents);
	}

	// Recalculates the gamma table according to the given gamma value
	// This is called on first startup or if the user changes the value
	void resampleGamma(float gamma) {
		int i,inf;
		
		// Linear gamma, just fill the array linearly 
		if (gamma == 1.0) {
			for (i = 0; i < 256; i++)
				_gammaTable[i] = i;
		}
		else {
			// Calculate the gamma values
			for (i = 0; i < 256; i++) {
				inf = (int)(255 * pow( static_cast<double>((i + 0.5) / 255.5) , static_cast<double>(gamma)) + 0.5);
				
				// Constrain the values to (0..255)
				if (inf < 0) {
					inf = 0;
				}
				else if (inf > 255) {
					inf = 255;
				}
				
				_gammaTable[i] = inf;
			}
		}
	}

};

void Textures_Realise()
{
  GlobalTexturesCache().setTextureParameters();
  GlobalTexturesCache().realise();
}

void Textures_Unrealise()
{
  GlobalTexturesCache().unrealise();
}

#include "modulesystem/modulesmap.h"
#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class TexturesDependencies :
	public GlobalRadiantModuleRef,
	public GlobalRegistryModuleRef,
	public GlobalOpenGLModuleRef,
	public GlobalPreferenceSystemModuleRef 
{
	ImageModulesRef m_image_modules;
	// The reference collection of ImageLoaders 
	ImageLoaderModulesRef _imageLoaders;
public:
	TexturesDependencies() :
		//m_image_modules(GlobalRadiant().getRequiredGameDescriptionKeyValue("texturetypes")),
		m_image_modules("jpg dds"),
		_imageLoaders("tga")
	{}
	
	ImageModules& getImageModules() {
		return m_image_modules.get();
	}
	
	// Returns the module collection of ImageLoaders
	ImageLoaderModules& getImageLoaders() {
		return _imageLoaders.get();
	}
};

class TexturesAPI
{
	TexturesMap* _texturesMap;
public:
	typedef TexturesCache Type;
	STRING_CONSTANT(Name, "*");

	TexturesAPI() {
		// Instantiate the class on the heap
		_texturesMap = new TexturesMap;
	}
	
	~TexturesAPI() {
		delete _texturesMap;
	}
	
	TexturesCache* getTable() {
		return _texturesMap;
	}
};

typedef SingletonModule<TexturesAPI, TexturesDependencies> TexturesModule;
typedef Static<TexturesModule> StaticTexturesModule;
StaticRegisterModule staticRegisterTextures(StaticTexturesModule::instance());

ImageModules& Textures_getImageModules() {
	return StaticTexturesModule::instance().getDependencies().getImageModules();
}

ImageLoaderModules& Textures_getImageLoaders() {
	return StaticTexturesModule::instance().getDependencies().getImageLoaders();
}
