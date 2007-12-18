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
#include "modelcache/ModelCache.h"

#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>
#include "modelcache/ModelResource.h"
#include "map/MapResource.h"

void MapChanged()
{
	GlobalMap().setModified(!References_Saved());
}

bool MapResource_loadFile(const MapFormat& format, scene::INodePtr root, const std::string& filename)
{
  globalOutputStream() << "Open file " << filename.c_str() << " for read...";
  TextFileInputStream file(filename);
  if(!file.failed())
  {
    globalOutputStream() << "success\n";
    format.readGraph(root, file);
    return true;
  }
  else
  {
    globalErrorStream() << "failure\n";
    return false;
  }
}

scene::INodePtr MapResource_load(const MapFormat& format, const std::string& path, const std::string& name)
{
  scene::INodePtr root(NewMapRoot(name));

  	std::string fullpath = path + name;
  	
  if(path_is_absolute(fullpath.c_str()))
  {
    MapResource_loadFile(format, root, fullpath);
  }
  else
  {
    globalErrorStream() << "map path is not fully qualified: " << makeQuoted(fullpath.c_str()) << "\n";
  }

  return root;
}

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

bool file_saveBackup(const char* path)
{
  if(file_writeable(path))
  {
	  StringOutputStream backup(256);
    backup << StringRange(path, path_get_extension(path)) << "bak";

    return (!file_exists(backup.c_str()) || file_remove(backup.c_str())) // remove backup
      && file_move(path, backup.c_str()); // rename current to backup
  }

  globalErrorStream() << "map path is not writeable: " << makeQuoted(path) << "\n";
  return false;
}

/** 
 * Save a map file (outer function). This function tries to backup the map
 * file before calling MapResource_saveFile() to do the actual saving of
 * data.
 */

bool MapResource_save(const MapFormat& format, 
					  scene::INodePtr root, 
					  const std::string& path, const std::string& name)
{
	std::string fullpath = path + name;
	
	if(path_is_absolute(fullpath.c_str())) {

		// Save a backup if possible. This is done by renaming the original,
		// which won't work if the existing map is currently open by Doom 3
		// in the background.
		if (file_exists(fullpath.c_str()) 
		    	&& !file_saveBackup(fullpath.c_str())) {
			globalErrorStream() << "WARNING: could not rename " 
								   << makeQuoted(fullpath.c_str())
								   << " to backup.\n";
		}
	
		// Save the actual file
		return MapResource_saveFile(format, root, map::traverse, fullpath.c_str());
	}
	else {
		globalErrorStream() << "map path is not fully qualified: " << makeQuoted(fullpath.c_str()) << "\n";
		return false;
	}
}

class HashtableReferenceCache 
: public ReferenceCache, 
  public VirtualFileSystem::Observer
{
public:
	// Map of named ModelResource objects
	typedef std::map<std::string, model::ModelResourceWeakPtr> ModelReferences;
	ModelReferences _modelReferences;
	
	// Map of named MapResource objects
	typedef std::map<std::string, map::MapResourceWeakPtr> MapReferences;
	MapReferences _mapReferences;
  
	bool _realised;

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_REFERENCECACHE);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_FILETYPES);
			_dependencies.insert("Doom3MapLoader");
			_dependencies.insert(MODULE_MODELLOADER + "ASE");
			_dependencies.insert(MODULE_MODELLOADER + "MD5MESH");
			_dependencies.insert(MODULE_MODELLOADER + "LWO");
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "ReferenceCache::initialiseModule called.\n";
		
		GlobalFileSystem().addObserver(*this);
		realise();
	}
	
	virtual void shutdownModule() {
		unrealise();
		GlobalFileSystem().removeObserver(*this);
	}
	
	HashtableReferenceCache() : 
		_realised(false)
	{}

  void clear()
  {
	  _modelReferences.clear();
	  _mapReferences.clear();
  }

    // Branch for capturing model resources
  	ResourcePtr captureModel(const std::string& path) {
  		// First lookup the reference in the map. If it is found, we need to
		// lock the weak_ptr to get a shared_ptr, which may fail. If we cannot
		// get a shared_ptr (because the object as already been deleted) or the
		// item is not found at all, we create a new ModelResource and add it
		// into the map before returning.
		ModelReferences::iterator i = _modelReferences.find(path);
		if (i != _modelReferences.end()) {
			// Found. Try to lock the pointer. If it is valid, return it.
			model::ModelResourcePtr candidate = i->second.lock();
			if (candidate) {
				return candidate;
			}
		}
		
		// Either we did not find the resource, or the pointer was not valid.
		// In this case we create a new ModelResource, add it to the map and
		// return it.
		model::ModelResourcePtr newResource(new model::ModelResource(path));
		
		// Realise the new resource if the ReferenceCache itself is realised
		if (realised()) {
			newResource->realise();
		}
		
		// Insert the weak pointer reference into the map
		_modelReferences[path] = model::ModelResourceWeakPtr(newResource);
		
		return newResource;
  	}
  	
  	// Branch for capturing mapfile resources
  	ResourcePtr captureMap(const std::string& path) {
  		// First lookup the reference in the map. If it is found, we need to
		// lock the weak_ptr to get a shared_ptr, which may fail. If we cannot
		// get a shared_ptr (because the object as already been deleted) or the
		// item is not found at all, we create a new ModelResource and add it
		// into the map before returning.
		MapReferences::iterator i = _mapReferences.find(path);
		if (i != _mapReferences.end()) {
			// Found. Try to lock the pointer. If it is valid, return it.
			map::MapResourcePtr candidate = i->second.lock();
			if (candidate) {
				return candidate;
			}
		}
		
		// Either we did not find the resource, or the pointer was not valid.
		// In this case we create a new ModelResource, add it to the map and
		// return it.
		map::MapResourcePtr newResource(new map::MapResource(path));
		
		// Realise the new resource if the ReferenceCache itself is realised
		if (realised()) {
			newResource->realise();
		}
		
		// Insert the weak pointer reference into the map
		_mapReferences[path] = map::MapResourceWeakPtr(newResource);
		
		return newResource;
  	}
  
	/*
	 * Capture a named resource.
	 */
	ResourcePtr capture(const std::string& path) {
		// Branch off to the map/model capture routines
		if (boost::algorithm::iends_with(path, "map")) {
			return captureMap(path);
		}
		else {
			return captureModel(path);
		}
	}
	
	bool realised() const {
		return _realised;
	}
	
	void realise() {
		ASSERT_MESSAGE(!_realised, "HashtableReferenceCache::realise: already realised");
		if (!_realised) {
			_realised = true;

			// Realise ModelResources
			for (ModelReferences::iterator i = _modelReferences.begin(); 
				 i != _modelReferences.end(); 
				 ++i)
			{
				model::ModelResourcePtr res = i->second.lock();
				if (res)
					res->realise();
			}
			
			for (MapReferences::iterator i = _mapReferences.begin();
		  	     i != _mapReferences.end();
		  	     ++i)
			{
				map::MapResourcePtr resource = i->second.lock();
	      		if (resource) {
	        		resource->realise();
	      		}
	    	}
		}
	}
	
	void unrealise() {
		if (_realised) {
			_realised = false;

			// Unrealise ModelResources
			for (ModelReferences::iterator i = _modelReferences.begin(); 
				 i != _modelReferences.end(); 
				 ++i)
			{
				model::ModelResourcePtr res = i->second.lock();
				if (res) {
					res->unrealise();
				}
			}
			
			for (MapReferences::iterator i = _mapReferences.begin();
		  	     i != _mapReferences.end();
		  	     ++i)
			{
				map::MapResourcePtr resource = i->second.lock();
	      		if (resource) {
	        		resource->unrealise();
	      		}
	    	}

			model::ModelCache::Instance().clear();
		}
	}

	// Gets called on VFS initialise
  	virtual void onFileSystemInitialise() {
  		realise();
  	}
  	
  	// Gets called on VFS shutdown
  	virtual void onFileSystemShutdown() {
  		unrealise();
  	}
  
	void refresh() {
		for (ModelReferences::iterator i = _modelReferences.begin();
	  	     i != _modelReferences.end();
	  	     ++i)
		{
			model::ModelResourcePtr resource = i->second.lock();
      		if (resource != NULL) {
        		resource->refresh();
      		}
    	}
		
		for (MapReferences::iterator i = _mapReferences.begin();
	  	     i != _mapReferences.end();
	  	     ++i)
		{
			map::MapResourcePtr resource = i->second.lock();
      		if (resource && !resource->isMap()) {
        		resource->refresh();
      		}
    	}
	}
};

// Define the ReferenceCache registerable module
module::StaticModule<HashtableReferenceCache> referenceCacheModule;

namespace
{
	HashtableReferenceCache& GetReferenceCache() {
		return static_cast<HashtableReferenceCache&>(
			  *referenceCacheModule.getModule()
		);
	}
}

void SaveReferences()
{
	for (HashtableReferenceCache::ModelReferences::iterator i = GetReferenceCache()._modelReferences.begin(); 
		 i != GetReferenceCache()._modelReferences.end(); 
		 ++i)
	{
    	model::ModelResourcePtr res = i->second.lock();
    	if (res)
    		res->save();
	}
	
	for (HashtableReferenceCache::MapReferences::iterator i = GetReferenceCache()._mapReferences.begin(); 
		 i != GetReferenceCache()._mapReferences.end(); 
		 ++i)
	{
    	map::MapResourcePtr res = i->second.lock();
    	if (res)
    		res->save();
	}
	
	MapChanged();
}

bool References_Saved()
{
	for (HashtableReferenceCache::ModelReferences::iterator i = GetReferenceCache()._modelReferences.begin(); 
		 i != GetReferenceCache()._modelReferences.end(); 
		 ++i)
	{
		scene::INodePtr node;
		    
	    model::ModelResourcePtr res = i->second.lock();
	    if (res)
	    	node = res->getNode();
	    	
	    if (node != NULL) {
	      MapFilePtr map = Node_getMapFile(node);
	      if(map != NULL && !map->saved())
	      {
	        return false;
	      }
	    }
	}
	
	for (HashtableReferenceCache::MapReferences::iterator i = GetReferenceCache()._mapReferences.begin(); 
		 i != GetReferenceCache()._mapReferences.end(); 
		 ++i)
	{
		scene::INodePtr node;
		    
	    map::MapResourcePtr res = i->second.lock();
	    if (res)
	    	node = res->getNode();
	    	
	    if (node != NULL) {
	      MapFilePtr map = Node_getMapFile(node);
	      if(map != NULL && !map->saved())
	      {
	        return false;
	      }
	    }
	}

	return true;
}

void RefreshReferences()
{
  ScopeDisableScreenUpdates disableScreenUpdates("Refreshing models");
  GetReferenceCache().refresh();
  // greebo: Reload the modelselector too
  ui::ModelSelector::refresh();
}

void FlushReferences()
{
	model::ModelCache::Instance().clear();
  GetReferenceCache().clear();
}
