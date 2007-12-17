#ifndef MODELCACHE_H_
#define MODELCACHE_H_

#include "imodelcache.h"
#include <map>

namespace model {

class ModelCache :
	public IModelCache
{
	typedef std::map<std::string, scene::INodePtr> ModelNodeMap;
	ModelNodeMap _modelNodeMap;

public:
	/**
	 * greebo: Looks up the model with the given path in the cache.
	 * 
	 * @returns: The singleton model NodePtr or NULL if not found.
	 */
	virtual scene::INodePtr find(const std::string& path);
	
	/**
	 * greebo: Inserts the given node into the modelcache. 
	 */
	virtual void insert(const std::string& path, const scene::INodePtr& modelNode);
	
	/**
	 * greebo: Removes the given path from the cache.
	 */
	virtual void erase(const std::string& path);
	
	// Clears the cache
	virtual void clear();
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

} // namespace model

#endif /*MODELCACHE_H_*/
