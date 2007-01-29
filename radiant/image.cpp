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

#include "image.h"

#include "modulesystem.h"
#include "iimage.h"
#include "ifilesystem.h"
#include "iarchive.h"

#include "generic/reference.h"
#include "os/path.h"
#include "stream/stringstream.h"

typedef Modules<_QERPlugImageTable> ImageModules;
ImageModules& Textures_getImageModules();

class LoadImageVisitor : public ImageModules::Visitor 
{
	// The filename to load
	const std::string _name;
	
	// The reference to the pointer in the parent function
	Image*& _image;
	
public:
	// Constructor
	LoadImageVisitor(const std::string& name, Image*& image) : 
		_name(name), 
		_image(image)
	{}

	// Visit function called for each image module. Provides the file extension and
	// a table with the loadImage function.
	void visit(const char* extension, const _QERPlugImageTable& table) {
		// Only do anything, if the image pointer is still NULL (i.e. the image load has not succeeded yet)
		if (_image == NULL) {
			// Construct the full name of the image to load, including the prefix (e.g. "dds/")
			// and the file extension.
			std::string fullName = table.prefix + std::string(_name) + "." + extension;
			
			// Try to open the file (will fail if the extension does not fit)
			ArchiveFile* file = GlobalFileSystem().openFile(fullName.c_str());
			
			// Has the file been loaded?
			if (file != NULL) {
				// Try to invoke the imageloader with a reference to the ArchiveFile
				_image = table.loadImage(*file);
				
				// Release the loaded file
				file->release();
			}
		}
	}
}; // class LoadImageVisitor

/// \brief Returns a new image for the first file matching \p name in one of the available texture formats, or 0 if no file is found.
Image* QERApp_LoadImage(void* environment, const char* name) {

	Image* image = NULL;

	// Instantiate a visitor class
	LoadImageVisitor visitor(name, image);

	// Cycle through all modules and tell them to visit the local class
	Textures_getImageModules().foreachModule(visitor);
	
	return image;
}
