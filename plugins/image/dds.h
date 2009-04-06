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

#if !defined(INCLUDED_DDS_H)
#define INCLUDED_DDS_H

#include "ifilesystem.h"
#include "iimage.h"
#include "imagelib.h" // for RGBAImagePtr
#include <iostream>

ImagePtr LoadDDS(ArchiveFile& file);

/* greebo: A DDSLoader is capable of loading DDS image files.
 *  
 * Use load() to actually retrieve an Image* object with the loaded image.
 * 
 * Shouldn't be used to load textures directly, use the 
 * GlobalMaterialManager() module instead.  
 * 
 * Complies with the ImageLoader interface defined in "iimage.h" 
 */
class DDSLoader :
	public ImageLoader
{
public:
	/* greebo: This loads the file and returns the pointer to 
	 * the allocated Image object (or an empty pointer, if the load failed). 
	 */
	ImagePtr load(ArchiveFile& file) const {
		// Pass the call to the according load function
		return LoadDDS(file);
	}
	
	/* greebo: Gets the file extension of the supported image file type (e.g. "dds") 
	 */
	std::string getExtension() const {
		return "dds";
	}
	
	/* greebo: Returns the prefix that is necessary to construct the
	 * path to the dds files. 
	 */
	std::string getPrefix() const {
		return "dds/";
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ImageLoaderDDS");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "ImageLoaderDDS::initialiseModule called.\n";
	}
};

#endif
