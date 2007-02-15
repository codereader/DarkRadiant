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
MapModules& ReferenceAPI_getMapModules();
#include "imodel.h"
ModelModules& ReferenceAPI_getModelModules();
#include "ifilesystem.h"
#include "iarchive.h"
#include "ifiletypes.h"
#include "ireference.h"
#include "ientity.h"
#include "qerplugin.h"

#include <list>
#include <fstream>

#include "container/cache.h"
#include "container/hashfunc.h"
#include "os/path.h"
#include "stream/textfilestream.h"
#include "nullmodel.h"
#include "maplib.h"
#include "stream/stringstream.h"
#include "os/file.h"
#include "moduleobserver.h"
#include "moduleobservers.h"

#include "mainframe.h"
#include "map.h"
#include "filetypes.h"

#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>

bool References_Saved();

void MapChanged()
{
	map::setModified(!References_Saved());
}


EntityCreator* g_entityCreator = 0;

bool MapResource_loadFile(const MapFormat& format, scene::Node& root, const char* filename)
{
  globalOutputStream() << "Open file " << filename << " for read...";
  TextFileInputStream file(filename);
  if(!file.failed())
  {
    globalOutputStream() << "success\n";
    ASSERT_NOTNULL(g_entityCreator);
    format.readGraph(root, file, *g_entityCreator);
    return true;
  }
  else
  {
    globalErrorStream() << "failure\n";
    return false;
  }
}

NodeSmartReference MapResource_load(const MapFormat& format, const char* path, const char* name)
{
  NodeSmartReference root(NewMapRoot(name));

  StringOutputStream fullpath(256);
  fullpath << path << name;

  if(path_is_absolute(fullpath.c_str()))
  {
    MapResource_loadFile(format, root, fullpath.c_str());
  }
  else
  {
    globalErrorStream() << "map path is not fully qualified: " << makeQuoted(fullpath.c_str()) << "\n";
  }

  return root;
}

/** Save the map contents to the given filename using the given MapFormat export module
 */
bool MapResource_saveFile(const MapFormat& format, scene::Node& root, GraphTraversalFunc traverse, const char* filename)
{
	globalOutputStream() << "Open file " << filename << " for write...";
	
	// Open the stream to the output file
	std::ofstream outfile(filename);

	if(outfile.is_open()) {
	    globalOutputStream() << "success\n";
	    ScopeDisableScreenUpdates disableScreenUpdates(path_get_filename_start(filename));

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

/** Save a map file (outer function). This function tries to backup the map
 * file before calling MapResource_saveFile() to do the actual saving of
 * data.
 */

bool MapResource_save(const MapFormat& format, scene::Node& root, const char* path, const char* name)
{
  StringOutputStream fullpath(256);
  fullpath << path << name;

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
		return MapResource_saveFile(format, root, Map_Traverse, fullpath.c_str());
	}
	else {
		globalErrorStream() << "map path is not fully qualified: " << makeQuoted(fullpath.c_str()) << "\n";
		return false;
	}
}

namespace
{
  NodeSmartReference g_nullNode(NewNullNode());
  NodeSmartReference g_nullModel(g_nullNode);
}

class NullModelLoader : public ModelLoader
{
public:
  scene::Node& loadModel(ArchiveFile& file)
  {
    return g_nullModel;
  }
  
  	// Required function, not implemented.
	model::IModelPtr loadModelFromPath(const std::string& name) {
		return model::IModelPtr();
	}
  
};

namespace
{
  NullModelLoader g_NullModelLoader;
}


/// \brief Returns the model loader for the model \p type or 0 if the model \p type has no loader module
ModelLoader* ModelLoader_forType(const char* type)
{
  std::string moduleName = findModuleName(&GlobalFiletypes(), 
  										  std::string(ModelLoader::Name()),
  										  type);
  if(!moduleName.empty())
  {
    ModelLoader* table = ReferenceAPI_getModelModules().findModule(moduleName.c_str());
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

NodeSmartReference ModelResource_load(ModelLoader* loader, const char* name)
{
  NodeSmartReference model(g_nullModel);

  {
    ArchiveFile* file = GlobalFileSystem().openFile(name);

    if(file != 0)
    {
      globalOutputStream() << "Loaded Model: \"" << name << "\"\n";
      model = loader->loadModel(*file);
      file->release();
    }
    else
    {
      globalErrorStream() << "Model load failed: \"" << name << "\"\n";
    }
  }

  model.get().m_isRoot = true;

  return model;
}


inline hash_t path_hash(const char* path, hash_t previous = 0)
{
#if defined(WIN32)
  return string_hash_nocase(path, previous);
#else // UNIX
  return string_hash(path, previous);
#endif
}

struct PathEqual
{
  bool operator()(const CopiedString& path, const CopiedString& other) const
  {
    return path_equal(path.c_str(), other.c_str());
  }
};

struct PathHash
{
  typedef hash_t hash_type;
  hash_type operator()(const CopiedString& path) const
  {
    return path_hash(path.c_str());
  }
};

typedef std::pair<CopiedString, CopiedString> ModelKey;

struct ModelKeyEqual
{
  bool operator()(const ModelKey& key, const ModelKey& other) const
  {
    return path_equal(key.first.c_str(), other.first.c_str()) && path_equal(key.second.c_str(), other.second.c_str());
  }
};

struct ModelKeyHash
{
  typedef hash_t hash_type;
  hash_type operator()(const ModelKey& key) const
  {
    return hash_combine(path_hash(key.first.c_str()), path_hash(key.second.c_str()));
  }
};

typedef HashTable<ModelKey, NodeSmartReference, ModelKeyHash, ModelKeyEqual> ModelCache;
ModelCache g_modelCache;
bool g_modelCache_enabled = true;

ModelCache::iterator ModelCache_find(const char* path, const char* name)
{
  if(g_modelCache_enabled)
  {
    return g_modelCache.find(ModelKey(path, name));
  }
  return g_modelCache.end();
}

ModelCache::iterator ModelCache_insert(const char* path, const char* name, scene::Node& node)
{
  if(g_modelCache_enabled)
  {
    return g_modelCache.insert(ModelKey(path, name), NodeSmartReference(node));
  }
  return g_modelCache.insert(ModelKey("", ""), g_nullModel);
}

void ModelCache_flush(const char* path, const char* name)
{
  ModelCache::iterator i = g_modelCache.find(ModelKey(path, name));
  if(i != g_modelCache.end())
  {
    //ASSERT_MESSAGE((*i).value.getCount() == 0, "resource flushed while still in use: " << (*i).key.first.c_str() << (*i).key.second.c_str());
    g_modelCache.erase(i);
  }
}

void ModelCache_clear()
{
  g_modelCache_enabled = false;
  g_modelCache.clear();
  g_modelCache_enabled = true;
}

NodeSmartReference Model_load(ModelLoader* loader, const char* path, const char* name, const char* type)
{
	// Model types should have a loader, so use this to load. Map types do not
	// have a loader		  
	if(loader != 0) {
		return ModelResource_load(loader, name);
	}
	else {
		// Get a loader module name for this type, if possible. If none is 
		// found, try again with the "map" type, since we might be loading a 
		// map with a different extension
	    std::string moduleName = findModuleName(&GlobalFiletypes(), 
	    										std::string(MapFormat::Name()), 
	    										type);
		// Empty, try again with "map" type
		if (moduleName.empty()) {
			moduleName = findModuleName(&GlobalFiletypes(), 
										std::string(MapFormat::Name()),
										"map"); 
		}
	
		// If we have a module, use it to load the map if possible, otherwise 
		// return an error
	    if (!moduleName.empty()) {
	      
			const MapFormat* format = ReferenceAPI_getMapModules().findModule(
										moduleName.c_str());
										
	      if(format != 0)
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
	      if(string_not_empty(type))
	      {
	        globalErrorStream() << "Model type not supported: \"" << name << "\"\n";
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
  NodeSmartReference m_model;
  
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

  void setModel(const NodeSmartReference& model)
  {
    m_model = model;
  }
  void clearModel()
  {
    m_model = g_nullModel;
  }

  void loadCached()
  {
    if(g_modelCache_enabled)
    {
      // cache lookup
      ModelCache::iterator i = ModelCache_find(m_path.c_str(), m_name.c_str());
      if(i == g_modelCache.end())
      {
        i = ModelCache_insert(
          m_path.c_str(),
          m_name.c_str(),
          Model_load(m_loader, m_path.c_str(), m_name.c_str(), _type.c_str())
        );
      }

      setModel((*i).value);
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
  bool save()
  {
//  	std::cout << "[referencecache.cpp] ModelResource::save() - " << m_name.c_str() << std::endl;
    if(!mapSaved())
    {
      std::string moduleName = findModuleName(GetFileTypeRegistry(), 
      										  std::string(MapFormat::Name()),
      										  _type);
      if(!moduleName.empty())
      {
        const MapFormat* format = ReferenceAPI_getMapModules().findModule(moduleName.c_str());
        if(format != 0 && MapResource_save(*format, m_model.get(), m_path.c_str(), m_name.c_str()))
        {
          mapSave();
          return true;
        }
      }
    }
    return false;
  }
  void flush()
  {
    if(realised())
    {
      ModelCache_flush(m_path.c_str(), m_name.c_str());
    }
  }
  scene::Node* getNode()
  {
    //if(m_model != g_nullModel)
    {
      return m_model.get_pointer();
    }
    //return 0;
  }
  void setNode(scene::Node* node)
  {
    ModelCache::iterator i = ModelCache_find(m_path.c_str(), m_name.c_str());
    if(i != g_modelCache.end())
    {
      (*i).value = NodeSmartReference(*node);
    }
    setModel(NodeSmartReference(*node));

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
    MapFile* map = Node_getMapFile(m_model);
    if(map != 0)
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
    MapFile* map = Node_getMapFile(m_model);
    if(map != 0)
    {
      map->save();
    }
  }
  bool mapSaved() const
  {
    MapFile* map = Node_getMapFile(m_model);
    if(map != 0)
    {
      return m_modified == modified() && map->saved();
    }
    return true;
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

  void setEntityCreator(EntityCreator& entityCreator)
  {
    g_entityCreator = &entityCreator;
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

      ModelCache_clear();
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

namespace
{
  HashtableReferenceCache g_referenceCache;
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
  ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Saving Map");

	for (HashtableReferenceCache::iterator i = g_referenceCache.begin(); 
		 i != g_referenceCache.end(); 
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
  for(HashtableReferenceCache::iterator i = g_referenceCache.begin(); i != g_referenceCache.end(); ++i)
  {
    scene::Node* node = NULL;
    
    boost::shared_ptr<ModelResource> res = i->second.lock();
    if (res)
    	node = res->getNode();
    	
    if(node != 0)
    {
      MapFile* map = Node_getMapFile(*node);
      if(map != 0 && !map->saved())
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
  g_referenceCache.refresh();
}


void FlushReferences()
{
  ModelCache_clear();

  g_referenceCache.clear();
}

ReferenceCache& GetReferenceCache()
{
  return g_referenceCache;
}


#include "modulesystem/modulesmap.h"
#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class ReferenceDependencies :
  public GlobalRadiantModuleRef,
  public GlobalFileSystemModuleRef,
  public GlobalFiletypesModuleRef
{
  ModelModulesRef m_model_modules;
  MapModulesRef m_map_modules;
public:
  ReferenceDependencies() :
    m_model_modules(GlobalRadiant().getRequiredGameDescriptionKeyValue("modeltypes")),
    m_map_modules(GlobalRadiant().getRequiredGameDescriptionKeyValue("maptypes"))
  {
  }
  ModelModules& getModelModules()
  {
    return m_model_modules.get();
  }
  MapModules& getMapModules()
  {
    return m_map_modules.get();
  }
};

class ReferenceAPI : public TypeSystemRef
{
  ReferenceCache* m_reference;
public:
  typedef ReferenceCache Type;
  STRING_CONSTANT(Name, "*");

  ReferenceAPI()
  {
    g_nullModel = NewNullModel();

    GlobalFileSystem().attach(g_referenceCache);

    m_reference = &GetReferenceCache();
  }
  ~ReferenceAPI()
  {
    GlobalFileSystem().detach(g_referenceCache);

    g_nullModel = g_nullNode;
  }
  ReferenceCache* getTable()
  {
    return m_reference;
  }
};

typedef SingletonModule<ReferenceAPI, ReferenceDependencies> ReferenceModule;
typedef Static<ReferenceModule> StaticReferenceModule;
StaticRegisterModule staticRegisterReference(StaticReferenceModule::instance());

ModelModules& ReferenceAPI_getModelModules()
{
  return StaticReferenceModule::instance().getDependencies().getModelModules();
}
MapModules& ReferenceAPI_getMapModules()
{
  return StaticReferenceModule::instance().getDependencies().getMapModules();
}
