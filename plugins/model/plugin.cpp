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

#include "plugin.h"

#include "itextstream.h"
#include "ifilesystem.h"
#include <stdio.h>
#include "picomodel.h"
typedef unsigned char byte;
#include <boost/algorithm/string/case_conv.hpp>

void PicoPrintFunc( int level, const char *str )
{
	if( str == 0 )
		return;
	switch( level )
	{
		case PICO_NORMAL:
			globalOutputStream() << str << "\n";
			break;
		
		case PICO_VERBOSE:
			//globalOutputStream() << "PICO_VERBOSE: " << str << "\n";
			break;
		
		case PICO_WARNING:
			globalErrorStream() << "PICO_WARNING: " << str << "\n";
			break;
		
		case PICO_ERROR:
			globalErrorStream() << "PICO_ERROR: " << str << "\n";
			break;
		
		case PICO_FATAL:
			globalErrorStream() << "PICO_FATAL: " << str << "\n";
			break;
	}
}

void PicoLoadFileFunc(char *name, byte **buffer, int *bufSize) {
	*bufSize = static_cast<int>(GlobalFileSystem().loadFile(name, (void**)buffer));
}

void PicoFreeFileFunc( void* file ) {
	GlobalFileSystem().freeFile(file);
}

void pico_initialise()
{
	PicoInit();
	PicoSetMallocFunc( malloc );
	PicoSetFreeFunc( free );
	PicoSetPrintFunc( PicoPrintFunc );
	PicoSetLoadFileFunc( PicoLoadFileFunc );
	PicoSetFreeFileFunc( PicoFreeFileFunc );
}

#include "PicoModelLoader.h"

// DarkRadiant module entry point
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	
	pico_initialise();

	const picoModule_t** modules = PicoModuleList( 0 );
	
	while (*modules != 0) {
		const picoModule_t* module = *modules++;
		
		if (module->canload && module->load)	{
			for (char*const* ext = module->defaultExts; *ext != 0; ++ext) {
				// greebo: File extension is expected to be UPPERCASE
				std::string extension(*ext);
				boost::algorithm::to_upper(extension);
				
				registry.registerModule(
					model::PicoModelLoaderPtr(new model::PicoModelLoader(module, extension))
				);
			}
		}
	}
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
