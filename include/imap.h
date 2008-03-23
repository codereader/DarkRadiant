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

#if !defined(INCLUDED_IMAP_H)
#define INCLUDED_IMAP_H

#include "imodule.h"
#include "inode.h"

// Forward declaration
namespace parser { class DefTokeniser; }

/// \brief A node whose state can be imported from a token stream.
class MapImporter
{
public:
    virtual bool importTokens(parser::DefTokeniser& tokeniser) = 0;
};
typedef boost::shared_ptr<MapImporter> MapImporterPtr;

/** A type of Node whose state can be written to an output stream.
 */
class MapExporter
{
public:
	/** Export this Node's state to the provided output stream.
	 */
	virtual void exportTokens(std::ostream& os) const = 0;
};
typedef boost::shared_ptr<MapExporter> MapExporterPtr;

/** Callback function to control how the Walker traverses the scene graph. This function
 * will be provided to the map export module by the Radiant map code.
 */
typedef void (*GraphTraversalFunc) (scene::INodePtr root, scene::NodeVisitor& walker);

namespace map {

// The MapExport/MapImport information structures, needed by the MapFormat::write() and
// MapFormat::read() methods. The actual definition of this structure is stored in the 
// files libs/MapExportInfo.h and libs/MapImportInfo.h.
class MapExportInfo;
class MapImportInfo;

} // namespace map

/** 
 * Map Format interface. Each map format is able to traverse the scene graph and write
 * the contents into a mapfile, or to load a mapfile and populate a scene graph.
 */
class MapFormat :
	public RegisterableModule
{
public:
	/**
	 * Read the contents of the given streams (which are contained in MapImportInfo)
	 * and add them as children to the given root node (also in MapImportInfo).
	 */
	virtual void readGraph(const map::MapImportInfo& importInfo) const = 0;

	/** Traverse the scene graph and write contents into the provided output stream.
	 * 
	 * @param exportInfo
	 * The MapExportInfo structure, which contains the ostream references and such.
	 */
	virtual void writeGraph(const map::MapExportInfo& exportInfo) const = 0;
};
typedef boost::shared_ptr<MapFormat> MapFormatPtr;

#endif
