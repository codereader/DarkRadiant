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

#include "referencecache.h"

#include "debugging/debugging.h"

#include "iscenegraph.h"
#include "iselection.h"
#include "iundo.h"
#include "imap.h"
#include "imodel.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "ifiletypes.h"
#include "ireference.h"
#include "ientity.h"
#include "iradiant.h"
#include "imodule.h"

#include <list>
#include <fstream>

#include "os/path.h"
#include "stream/textfilestream.h"
#include "nullmodel.h"
#include "stream/stringstream.h"
#include "os/file.h"
#include "moduleobserver.h"
#include "moduleobservers.h"
#include "modulesystem/StaticModule.h"
#include "modulesystem/ModuleRegistry.h"
#include "ui/modelselector/ModelSelector.h"

#include "map/RootNode.h"
#include "mainframe.h"
#include "map/Map.h"
#include "map/algorithm/Traverse.h"
#include "referencecache/ModelCache.h"

#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>
#include "referencecache/ModelResource.h"
#include "map/MapResource.h"

/** Save the map contents to the given filename using the given MapFormat export module
 */
bool MapResource_saveFile(const MapFormat& format, scene::INodePtr root, GraphTraversalFunc traverse, const char* filename)
{
	globalOutputStream() << "Open file " << filename << " for write...";
	
	// Open the stream to the output file
	std::ofstream outfile(filename);

	if(outfile.is_open()) {
	    globalOutputStream() << "success\n";
	    
		// Use the MapFormat module and traversal function to dump the scenegraph
		// to the file stream.
	    format.writeGraph(root, traverse, outfile);
	    
	    outfile.close();
	    return true;
	}
	else {
		globalErrorStream() << "failure\n";
		return false;
	}
}
