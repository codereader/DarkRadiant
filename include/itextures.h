/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

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

#if !defined(INCLUDED_ITEXTURES_H)
#define INCLUDED_ITEXTURES_H

#include "iimage.h"
#include "generic/constant.h"
#include "igl.h"
#include <string>

// Forward declaration
struct qtexture_t; // defined in texturelib.h

class LoadImageCallback
{
	// The type of a load function
	typedef Image* (*LoadFunc)(void* environment, const char* name);

public:
	void* m_environment;
	LoadFunc m_func;

	LoadImageCallback(void* environment, LoadFunc func) : 
		m_environment(environment), 
		m_func(func) 
	{}
	
	Image* loadImage(const std::string& name) const {
		return m_func(m_environment, name.c_str());
	}
};


inline bool operator==(const LoadImageCallback& self, const LoadImageCallback& other)
{
  return self.m_environment == other.m_environment && self.m_func == other.m_func; 
}
inline bool operator<(const LoadImageCallback& self, const LoadImageCallback& other)
{
  return self.m_environment < other.m_environment || 
        (!(other.m_environment < self.m_environment) && self.m_func < other.m_func); 
}

class TexturesCacheObserver
{
public:
  virtual void unrealise() = 0;
  virtual void realise() = 0;
};

/* greebo: A TextureModeObserver gets notified if the
 * texture mode gets changed. 
 */
class TextureModeObserver
{
public:
	virtual void textureModeChanged() = 0;
};

enum ETexturesMode {
    eTextures_NEAREST = 0,
    eTextures_NEAREST_MIPMAP_NEAREST = 1,
    eTextures_NEAREST_MIPMAP_LINEAR = 2,
    eTextures_LINEAR = 3,
    eTextures_LINEAR_MIPMAP_NEAREST = 4,
    eTextures_LINEAR_MIPMAP_LINEAR = 5,
};

class TexturesCache
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "textures");
  
	// Retrieves the default image loader
	virtual LoadImageCallback defaultLoader() const = 0;
  
	// Loads an image by using the default loader and returns the pointer
	virtual Image* loadImage(const std::string& name) = 0;
  
	/**
	 * Capture the named image texture and return the associated qtexture_t
	 * struct.
	 */
	virtual qtexture_t* capture(const std::string& name) = 0;
  
	/**
	 * Capture the named image texture using the provided image loader.
	 */
	virtual qtexture_t* capture(const LoadImageCallback& load, 
								const std::string& name) = 0;
	
  virtual void release(qtexture_t* texture) = 0;
  virtual void attach(TexturesCacheObserver& observer) = 0;
  virtual void detach(TexturesCacheObserver& observer) = 0;
  
  	// Realises / unrealises all the textures (loads them into memory)
	virtual void realise() = 0;
	virtual void unrealise() = 0;
	
	// Returns true if the textures have already been realised
	virtual bool realised() const = 0; 
	
	// Sets the openGL states according to the internal texturemode
	virtual void setTextureParameters() = 0;
	
	virtual void setTextureComponents(GLint textureComponents) = 0;
	
	// Get / Set the texture mode
	virtual ETexturesMode getTextureMode() const = 0;
	virtual void setTextureMode(ETexturesMode mode) = 0;
	
	// Notifies the observers of the texture mode change
	virtual void modeChanged() = 0;
	
	// Adds/Removes an observer that gets notified upon texture mode change
	virtual void addTextureModeObserver(TextureModeObserver* observer) = 0;
	virtual void removeTextureModeObserver(TextureModeObserver* observer) = 0;
};

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<TexturesCache> GlobalTexturesModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<TexturesCache> GlobalTexturesModuleRef;

inline TexturesCache& GlobalTexturesCache()
{
  return GlobalTexturesModule::getTable();
}

#endif
