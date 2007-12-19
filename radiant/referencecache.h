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

#if !defined (INCLUDED_REFERENCECACHE_H)
#define INCLUDED_REFERENCECACHE_H

/// \brief Saves all open resource references if they differ from the version on disk.
void SaveReferences();
/// \brief Flushes the cache of resource references. All resource references must be released before calling this.
void FlushReferences();
/// \brief Reloads all resource references that differ from the version on disk.
void RefreshReferences();

bool References_Saved();

#include "iscenegraph.h"

class MapFormat;
typedef void(*GraphTraversalFunc)(scene::INodePtr root, const scene::Traversable::Walker& walker);

scene::INodePtr MapResource_load(const MapFormat& format, const std::string& path, const std::string& name);
bool MapResource_saveFile(const MapFormat& format, scene::INodePtr root, GraphTraversalFunc traverse, const char* filename);

// Get the ModelLoader class for the given model type

class ModelLoader;
ModelLoader* ModelLoader_forType(const char* type);

#endif
