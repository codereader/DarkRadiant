#ifndef IMODELCACHE_H_
#define IMODELCACHE_H_

#include "imodule.h"
#include "inode.h"

namespace model {

class IModelCache :
	public RegisterableModule
{
public:
	/**
	 * greebo: Looks up the model with the given path in the cache.
	 * 
	 * @returns: The singleton model NodePtr or NULL if not found.
	 */
	virtual scene::INodePtr find(const std::string& path) = 0;
	
	/**
	 * greebo: Inserts the given model path (associated with the given node) 
	 *         into the modelcache. 
	 */
	virtual void insert(const std::string& path, const scene::INodePtr& modelNode) = 0;
	
	/**
	 * greebo: Removes the given path from the cache.
	 */
	virtual void erase(const std::string& path) = 0;
	
	/**
	 * greebo: Clears out all mappings.
	 */
	virtual void clear() = 0;
};

} // namespace model

const std::string MODULE_MODELCACHE("ModelCache");

// Accessor 
inline model::IModelCache& GlobalModelCache() {
	// Cache the reference locally
	static model::IModelCache& _modelCache(
		*boost::static_pointer_cast<model::IModelCache>(
			module::GlobalModuleRegistry().getModule(MODULE_MODELCACHE)
		)
	);
	return _modelCache;
}

#endif /*IMODELCACHE_H_*/
