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

#include <ostream>

/* FORWARD DECLS */
class TokenWriter;
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

#include "iscenegraph.h"

class EntityCreator;

class TextInputStream;

/** Callback function to control how the Walker traverses the scene graph. This function
 * will be provided to the map export module by the Radiant map code.
 */
typedef void (*GraphTraversalFunc) (scene::INodePtr root, const scene::Traversable::Walker& walker);

/** Map Format interface. Each map format is able to traverse the scene graph and write
 * the contents into a mapfile, or to load a mapfile and populate a scene graph.
 */
class MapFormat :
	public RegisterableModule
{
public:
	/// \brief Read a map graph into \p root from \p outputStream, using \p entityTable to create entities.
	virtual void readGraph(scene::INodePtr root, TextInputStream& inputStream, EntityCreator& entityTable) const = 0;

	/** Traverse the scene graph and write contents into the provided output stream.
	 * 
	 * @param root
	 * The root of the scenegraph to traverse.
	 * 
	 * @param traverse
	 * Pointer to a graph traversal function. This function takes as parameters the scenegraph
	 * root and a scene::Traversable::Walker subclass to traverse the graph.
	 * 
	 * @param oStream
	 * The output stream to write contents to.
	 */
	virtual void writeGraph(scene::INodePtr root, GraphTraversalFunc traverse, std::ostream& oStream) const = 0;
};

#endif
