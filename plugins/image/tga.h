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

#if !defined (INCLUDED_TGA_H)
#define INCLUDED_TGA_H

#include "ifilesystem.h"
#include "iimage.h"
#include "modulesystem/singletonmodule.h"

Image* LoadTGA(ArchiveFile& file);

/* greebo: A TGALoader is capable of loading TGA files.
 *  
 * Use load() to actually retrieve an Image* object with the loaded image.
 * 
 * Shouldn't be used to load textures directly, use the 
 * GlobalTexturesCache() module instead.  
 * 
 * Complies with the ImageLoader interface defined in "iimage.h" 
 */
class TGALoader :
	public ImageLoader
{
public:
	// Definitions to enable the lookup by the radiant modulesystem
	typedef ImageLoader Type;
	STRING_CONSTANT(Name, "tga");

	// Return the instance pointer
	ImageLoader* getTable() {
		return this;
	}

public:
	// Constructor
	TGALoader() {}
	
	/* greebo: This loads the file and returns the pointer to 
	 * the allocated Image object (or NULL, if the load failed). 
	 */
	Image* load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadTGA(file);
	}
	
	/* greebo: Gets the file extension of the supported image file type (e.g. "tga") 
	 */
	std::string getExtension() const {
		return "tga";
	}

};

// The dependency class
class TGALoaderDependencies :
	public GlobalFileSystemModuleRef
{};

// Define the TGA module with the TGALoader and its dependencies
typedef SingletonModule<TGALoader, TGALoaderDependencies> TGALoaderModule;

#endif

