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

#include "map/RootNode.h"
#include "mainframe.h"
#include "map/Map.h"
#include "map/algorithm/Traverse.h"

#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>

bool References_Saved();

void MapChanged()
{
	GlobalMap().setModified(!References_Saved());
}

bool MapResource_loadFile(const MapFormat& format, scene::INodePtr root, const std::string& filename)
{
  globalOutputStream() << "Open file " << filename.c_str() << " for read...";
  TextFileInputStream file(filename.c_str());
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

namespace
{
  scene::INodePtr g_nullNode(NewNullNode());
  scene::INodePtr g_nullModel(g_nullNode);
}

class NullModelLoader : 
	public ModelLoader
{
public:
	scene::INodePtr loadModel(ArchiveFile& file) {
		return g_nullModel;
	}
  
  	// Required function, not implemented.
	model::IModelPtr loadModelFromPath(const std::string& name) {
		return model::IModelPtr();
	}
  
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_MODELLOADER + "NULL");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies; // no dependencies
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << getName().c_str() << "::initialiseModule called.\n";
	}
};

namespace
{
  NullModelLoader g_NullModelLoader;
}


/// \brief Returns the model loader for the model \p type or 0 if the model \p type has no loader module
ModelLoader* ModelLoader_forType(const char* type)
{
  std::string moduleName = GlobalFiletypes().findModuleName("model", type);
  if(!moduleName.empty())
  {
    ModelLoader* table = boost::static_pointer_cast<ModelLoader>(
    	module::GlobalModuleRegistry().getModule(moduleName)
    ).get();
    
    if(table != 0)
    {
      return table;
    }
    else
    {
      globalErrorStream() << "ERROR: Model type incorrectly registered: \"" 
      					  << moduleName.c_str() << "\"\n";
      return &g_NullModelLoader;
    }
  }
  return 0;
}

scene::INodePtr ModelResource_load(ModelLoader* loader, const std::string& name)
{
  scene::INodePtr model(g_nullModel);

  {
    ArchiveFile* file = GlobalFileSystem().openFile(name.c_str());

    if(file != 0)
    {
      globalOutputStream() << "Loaded Model: \"" << name.c_str() << "\"\n";
      model = loader->loadModel(*file);
      file->release();
    }
    else
    {
      globalErrorStream() << "Model load failed: \"" << name.c_str() << "\"\n";
    }
  }

  model->setIsRoot(true);

  return model;
}

class ModelCache
{
	// The left-hand value of the ModelCache map
	typedef std::pair<std::string, std::string> ModelKey;
	
	// The ModelNodeCache maps path/name string-pairs to scene::Nodes
	typedef std::map<ModelKey, scene::INodePtr> ModelNodeCache;
	ModelNodeCache _cache;
	
	// True, if lookups in the modelcache should succeed
	// When this is set to FALSE, the lookups return the end() iterator
	bool _enabled;
	
public:
	ModelCache() :
		_enabled(true)
	{}
	
	// Define the public iterator (FIXME: is this necessary?)
	typedef ModelNodeCache::iterator iterator;
	
	iterator find(const std::string& path, const std::string& name) {
		if (_enabled) {
			return _cache.find(ModelKey(path, name));
		}

		// Not enabled, return the end() iterator in this case
		return _cache.end();
	}
	
	iterator insert(const std::string& path, const std::string& name, 
		scene::INodePtr node)
	{
		if (_enabled) {
			// Insert into map and save the iterator
			std::pair<ModelNodeCache::iterator, bool> i = _cache.insert(
				ModelNodeCache::value_type(ModelKey(path, name), node)
			);
			// Return the iterator to the inserted element
			return i.first; 
		}
		
		// Not enabled, return a Nullmodel entry iterator
		std::pair<ModelNodeCache::iterator, bool> i = _cache.insert(
			ModelNodeCache::value_type(ModelKey("", ""), g_nullModel)
		);
		return i.first;
	}
	
	void flush(const std::string& path, const std::string& name) {
		ModelNodeCache::iterator i = _cache.find(ModelKey(path, name));
		
		if (i != _cache.end()) {
			_cache.erase(i);
		}
	}
	
	void clear() {
		_enabled = false;
		_cache.clear();
		_enabled = true;
	}
	
	bool enabled() const {
		return _enabled;
	}
	
	iterator begin() {
		return _cache.begin();
	}
	
	iterator end() {
		return _cache.end();
	}
};

ModelCache g_modelCache;

scene::INodePtr Model_load(ModelLoader* loader, const std::string& path, const std::string& name, const std::string& type)
{
	// greebo: Check if we have a NULL model loader and an empty path ("func_static_637")
	if (loader == NULL && path.empty()) {
		return g_nullModel;
	}

	// Model types should have a loader, so use this to load. Map types do not
	// have a loader		  
	if (loader != 0) {
		return ModelResource_load(loader, name);
	}
	else {
		// Get a loader module name for this type, if possible. If none is 
		// found, try again with the "map" type, since we might be loading a 
		// map with a different extension
	    std::string moduleName = GlobalFiletypes().findModuleName("map", type);
		// Empty, try again with "map" type
		if (moduleName.empty()) {
			moduleName = GlobalFiletypes().findModuleName("map", "map"); 
		}
	
		// If we have a module, use it to load the map if possible, otherwise 
		// return an error
	    if (!moduleName.empty()) {
	      
			const MapFormat* format = boost::static_pointer_cast<MapFormat>(
				module::GlobalModuleRegistry().getModule(moduleName)
			).get();
										
	      if(format != NULL)
	      {
	        return MapResource_load(*format, path, name);
	      }
	      else
	      {
	        globalErrorStream() << "ERROR: Map type incorrectly registered: \"" << moduleName.c_str() << "\"\n";
	        return g_nullModel;
	      }
	    }
	    else
	    {
	      if (!type.empty())
	      {
	        globalErrorStream() << "Model type not supported: \"" << name.c_str() << "\"\n";
	      }
	      return g_nullModel;
	    }
	}
}

namespace
{
  bool g_realised = false;

  // name may be absolute or relative
  const char* rootPath(const char* name)
  {
    return GlobalFileSystem().findRoot(
      path_is_absolute(name)
        ? name
        : GlobalFileSystem().findFile(name)
    );
  }
}

struct ModelResource 
: public Resource,
  public boost::noncopyable
{
  scene::INodePtr m_model;
  
	// Name given during construction
	const std::string m_originalName;
	
  std::string m_path;
  std::string m_name;
  
	// Type of resource (map, lwo etc)
	std::string _type;
	
	// ModelLoader for this resource type
	ModelLoader* m_loader;
	
  ModuleObservers m_observers;
  std::time_t m_modified;
  std::size_t m_unrealised;

	// Constructor
	ModelResource(const std::string& name) 
	: m_model(g_nullModel),
	  m_originalName(name),
	  _type(name.substr(name.rfind(".") + 1)),
	  m_loader(0),
	  m_modified(0),
	  m_unrealised(1)
	{
		// Get the model loader for this resource type
		m_loader = ModelLoader_forType(_type.c_str());
					  
		// Realise self if the ReferenceCache is itself realised. TODO: evil
		// global variable, remove this
		if(g_realised) {
      		realise();
		}
	}
	
  ~ModelResource()
  {
    if(realised())
    {
      unrealise();
    }
    ASSERT_MESSAGE(!realised(), "ModelResource::~ModelResource: resource reference still realised: " << makeQuoted(m_name.c_str()));
  }

  void setModel(scene::INodePtr model)
  {
    m_model = model;
  }
  void clearModel()
  {
    m_model = g_nullModel;
  }

  void loadCached()
  {
    if(g_modelCache.enabled())
    {
      // cache lookup
      ModelCache::iterator i = g_modelCache.find(m_path, m_name);
      
      if (i == g_modelCache.end())
      {
    	  // Model was not found yet
    	  scene::INodePtr loaded = Model_load(m_loader, m_path, m_name, _type);
    	  
    	  // Update the iterator, it holds the newly loaded model entry now
    	  i = g_modelCache.insert(m_path, m_name, loaded);
      }

      setModel(i->second);
    }
    else
    {
      setModel(Model_load(m_loader, m_path.c_str(), m_name.c_str(), _type.c_str()));
    }
  }

  void loadModel()
  {
    loadCached();
    connectMap();
    mapSave();
  }

  bool load()
  {
    ASSERT_MESSAGE(realised(), "resource not realised");
    if(m_model == g_nullModel)
    {
      loadModel();
    }

    return m_model != g_nullModel;
  }
  
	/**
	 * Save this resource (only for map resources).
	 * 
	 * @returns
	 * true if the resource was saved, false otherwise.
	 */
	bool save() {
		std::string moduleName = GlobalFiletypes().findModuleName("map", _type);
  									
		if(!moduleName.empty()) {
			const MapFormat* format = boost::static_pointer_cast<MapFormat>(
				module::GlobalModuleRegistry().getModule(moduleName)
			).get();
			
			if (format != NULL && MapResource_save(*format, m_model, m_path, m_name)) {
      			mapSave();
      			return true;
    		}
  		}
  		
  		return false;
	}
	
  void flush()
  {
    if(realised())
    {
      g_modelCache.flush(m_path, m_name);
    }
  }
  scene::INodePtr getNode()
  {
    //if(m_model != g_nullModel)
    {
      return m_model;
    }
    //return 0;
  }
  void setNode(scene::INodePtr node)
  {
    ModelCache::iterator i = g_modelCache.find(m_path, m_name);
    if(i != g_modelCache.end())
    {
      i->second = node;
    }
    setModel(node);

    connectMap();
  }
  void attach(ModuleObserver& observer)
  {
    if(realised())
    {
      observer.realise();
    }
    m_observers.attach(observer);
  }
  void detach(ModuleObserver& observer)
  {
    if(realised())
    {
      observer.unrealise();
    }
    m_observers.detach(observer);
  }
  bool realised()
  {
    return m_unrealised == 0;
  }
  
	// Realise this ModelResource
	void realise() {
	    if(--m_unrealised == 0) {

    		m_path = rootPath(m_originalName.c_str());
    		m_name = path_make_relative(m_originalName.c_str(), m_path.c_str());

			// Realise the observers
			m_observers.realise();
		}
	}
	
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      m_observers.unrealise();

      //globalOutputStream() << "ModelResource::unrealise: " << m_path.c_str() << m_name.c_str() << "\n";
      clearModel();
    }
  }
  bool isMap() const
  {
    return Node_getMapFile(m_model) != 0;
  }
  void connectMap()
  {
    MapFilePtr map = Node_getMapFile(m_model);
    if(map != NULL)
    {
      map->setChangedCallback(FreeCaller<MapChanged>());
    }
  }
  std::time_t modified() const
  {
    StringOutputStream fullpath(256);
    fullpath << m_path.c_str() << m_name.c_str();
    return file_modified(fullpath.c_str());
  }
  void mapSave()
  {
    m_modified = modified();
    MapFilePtr map = Node_getMapFile(m_model);
    if(map != NULL)
    {
      map->save();
    }
  }

  bool isModified() const
  {
    return ((!string_empty(m_path.c_str()) // had or has an absolute path
        && m_modified != modified()) // AND disk timestamp changed
      || !path_equal(rootPath(m_originalName.c_str()), m_path.c_str())); // OR absolute vfs-root changed
  }
  void refresh()
  {
    if(isModified())
    {
      flush();
      unrealise();
      realise();
    }
  }
};

class HashtableReferenceCache 
: public ReferenceCache, 
  public ModuleObserver
{
	// Resource pointer types
	typedef boost::shared_ptr<ModelResource> ModelResourcePtr;
	typedef boost::weak_ptr<ModelResource> ModelResourceWeakPtr;
	
	// Map of named ModelResource objects
	typedef std::map<std::string, ModelResourceWeakPtr> ModelReferences;
	ModelReferences m_references;
  
	std::size_t m_unrealised;

public:

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_REFERENCECACHE);
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;
		
		// greebo: TODO: This list can probably be made smaller,
		// not all modules are necessary during initialisation
		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_RADIANT);
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_FILETYPES);
			_dependencies.insert("Doom3MapLoader");
			// Model Loaders?
		}
		
		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "ReferenceCache::initialiseModule called.\n";
		
		g_nullModel = NewNullModel();

		GlobalFileSystem().attach(*this);
	}
	
	virtual void shutdownModule() {
		GlobalFileSystem().detach(*this);

		g_nullModel = g_nullNode;
	}
	
  typedef ModelReferences::iterator iterator;

  HashtableReferenceCache() : m_unrealised(1)
  {
  }

  iterator begin()
  {
    return m_references.begin();
  }
  iterator end()
  {
    return m_references.end();
  }

  void clear()
  {
    m_references.clear();
  }

	/*
	 * Capture a named resource.
	 */
	ResourcePtr capture(const std::string& path) {
		
		// First lookup the reference in the map. If it is found, we need to
		// lock the weak_ptr to get a shared_ptr, which may fail. If we cannot
		// get a shared_ptr (because the object as already been deleted) or the
		// item is not found at all, we create a new ModelResource and add it
		// into the map before returning.
		ModelReferences::iterator i = m_references.find(path);
		if (i != m_references.end()) {
			// Found. Try to lock the pointer. If it is valid, return it.
			ModelResourcePtr candidate = i->second.lock();
			if (candidate) {
				return candidate;
			}
		}
		
		// Either we did not find the resource, or the pointer was not valid.
		// In this case we create a new ModelResource, add it to the map and
		// return it.
		ModelResourcePtr newResource(new ModelResource(path));
		m_references[path] = ModelResourceWeakPtr(newResource);
		return newResource;
	}
	
	/*
	 * Release a named resource.
	 */
	void release(const std::string& path) {
		// Does nothing. TODO: remove this or implement weak pointer references
	}

  bool realised() const
  {
    return m_unrealised == 0;
  }
  void realise()
  {
    ASSERT_MESSAGE(m_unrealised != 0, "HashtableReferenceCache::realise: already realised");
    if(--m_unrealised == 0)
    {
      	g_realised = true;
		
		// Realise ModelResources
		for (ModelReferences::iterator i = m_references.begin();
	  	     i != m_references.end();
	  	     ++i)
		{
			ModelResourcePtr res = i->second.lock();
			if (res)
				res->realise();
		}
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      g_realised = false;

		// Unrealise ModelResources
		for (ModelReferences::iterator i = m_references.begin();
	  	     i != m_references.end();
	  	     ++i)
		{
			ModelResourcePtr res = i->second.lock();
			if (res)
				res->unrealise();
		}

      g_modelCache.clear();
    }
  }
  void refresh()
  {
		for (ModelReferences::iterator i = m_references.begin();
	  	     i != m_references.end();
	  	     ++i)
		{
			ModelResourcePtr resource = i->second.lock();
      		if(resource && !resource->isMap()) {
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

#if 0
class ResourceVisitor
{
public:
  virtual void visit(const char* name, const char* path, const
};
#endif

void SaveReferences()
{
	for (HashtableReferenceCache::iterator i = GetReferenceCache().begin(); 
		 i != GetReferenceCache().end(); 
		 ++i)
	{
    	boost::shared_ptr<ModelResource> res = i->second.lock();
    	if (res)
    		res->save();
	}
	
	MapChanged();
}

bool References_Saved()
{
  for(HashtableReferenceCache::iterator i = GetReferenceCache().begin(); i != GetReferenceCache().end(); ++i)
  {
    scene::INodePtr node;
    
    boost::shared_ptr<ModelResource> res = i->second.lock();
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
}

void FlushReferences()
{
  g_modelCache.clear();
  GetReferenceCache().clear();
}
