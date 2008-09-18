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

#include "imodule.h"
#include "stream/textstream.h"

#include "jpeg.h"
#include "tga.h"
#include "bmp.h"
#include "pcx.h"
#include "dds.h"
#include "ImageGDK.h"

typedef boost::shared_ptr<TGALoader> TGALoaderPtr;
typedef boost::shared_ptr<JPGLoader> JPGLoaderPtr;
typedef boost::shared_ptr<PCXLoader> PCXLoaderPtr;
typedef boost::shared_ptr<BMPLoader> BMPLoaderPtr;
typedef boost::shared_ptr<DDSLoader> DDSLoaderPtr;
typedef boost::shared_ptr<GDKLoader> GDKLoaderPtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(TGALoaderPtr(new TGALoader));
	registry.registerModule(JPGLoaderPtr(new JPGLoader));
	registry.registerModule(PCXLoaderPtr(new PCXLoader));
	registry.registerModule(BMPLoaderPtr(new BMPLoader));
	registry.registerModule(DDSLoaderPtr(new DDSLoader));
	registry.registerModule(GDKLoaderPtr(new GDKLoader));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
