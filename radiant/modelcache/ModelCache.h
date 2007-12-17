#ifndef MODELCACHE_H_
#define MODELCACHE_H_

#include "imodelcache.h"

namespace model {

class ModelCache :
	public IModelCache
{
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
	virtual void insert(const scene::INodePtr& modelNode);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

} // namespace model

#endif /*MODELCACHE_H_*/
