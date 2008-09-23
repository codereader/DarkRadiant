#ifndef MODELCACHE_H_
#define MODELCACHE_H_

#include <map>
#include <string>
#include "imodelcache.h"

namespace model {

class ModelCache :
	public IModelCache
{
	// The container maps model names to weak references	
	typedef std::map<std::string, IModelWeakPtr> ModelMap;
	ModelMap _modelMap;

	// Flag to disable the cache on demand (used during clear())
	bool _enabled;
	
public:
	ModelCache();

	// greebo: For documentation, see the abstract base class.
	virtual scene::INodePtr getModelNode(const std::string& modelPath);

	// greebo: For documentation, see the abstract base class.
	virtual IModelPtr getModel(const std::string& modelPath);

	// greebo: Get a model loader for the given type (file extension).
	// This returns never NULL, there is always the NullModelLoader available.
	virtual ModelLoaderPtr getModelLoaderForType(const std::string& type);

	// Clears the cache
	virtual void clear();

	// Command target: this reloads all models in the map
	void refreshModels();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
};

} // namespace model

#endif /*MODELCACHE_H_*/
