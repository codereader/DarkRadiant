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
#include <boost/shared_ptr.hpp>

class ImageConstructor;
typedef boost::shared_ptr<ImageConstructor> ImageConstructorPtr;

// Forward declaration
struct Texture; // defined in texturelib.h
typedef boost::shared_ptr<Texture> TexturePtr;

/* greebo: A TextureModeObserver gets notified if the texture mode gets changed. 
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
  
	/**
	 * Capture the named image texture using the provided ImageConstructor.
	 */
	virtual TexturePtr capture(ImageConstructorPtr constructor, 
							   const std::string& name) = 0;
	
	virtual void release(TexturePtr texture) = 0;

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
