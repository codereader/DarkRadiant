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
#include "iregistry.h"
#include "iselection.h"
#include "iundo.h"
#include "imap.h"
#include "imodel.h"
#include "ifilesystem.h"
#include "iarchive.h"
#include "ifiletypes.h"
#include "ientity.h"
#include "iradiant.h"
#include "imodule.h"

#include <list>
#include <fstream>

#include "gtkutil/dialog.h"
#include "os/path.h"
#include "stream/textfilestream.h"
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
#include "map/MapResource.h"

#include "MapExportInfo.h"

/** Save the map contents to the given filename using the given MapFormat export module
 */
bool MapResource_saveFile(const MapFormat& format, scene::INodePtr root, GraphTraversalFunc traverse, const char* filename)
{
	globalOutputStream() << "Open file " << filename << " ";
	
	if (file_exists(filename) && !file_writeable(filename)) {
		// File is write-protected
		globalErrorStream() << "failure, file is write-protected." << std::endl;
		gtkutil::errorDialog(std::string("File is write-protected: ") + filename, GlobalRadiant().getMainWindow());
		return false;
	}

	// Open the stream to the output file
	std::ofstream outfile(filename);

	// Open the auxiliary file too
	std::string auxFilename(filename);
	auxFilename = auxFilename.substr(0, auxFilename.rfind('.'));
	auxFilename += GlobalRegistry().get(map::RKEY_INFO_FILE_EXTENSION);

	globalOutputStream() << "and auxiliary file " << auxFilename << " for write...";

	if (file_exists(auxFilename.c_str()) && !file_writeable(auxFilename.c_str())) {
		// File is write-protected
		globalErrorStream() << "failure, file is write-protected." << std::endl;
		gtkutil::errorDialog(std::string("File is write-protected: ") + auxFilename, GlobalRadiant().getMainWindow());
		return false;
	}

	std::ofstream auxfile(auxFilename.c_str());

	if (outfile.is_open() && auxfile.is_open()) {
	    globalOutputStream() << "success\n";

		map::MapExportInfo exportInfo(outfile, auxfile);
		exportInfo.traverse = traverse;
		exportInfo.root = root;
	    
		// Let the map exporter module do its job
	    format.writeGraph(exportInfo);
	    
	    outfile.close();
		auxfile.close();
	    return true;
	}
	else {
		globalErrorStream() << "failure\n";
		return false;
	}
}
