#ifndef MODELCACHE_H_
#define MODELCACHE_H_

#include <map>
#include <string>
#include "inode.h"

namespace model {

class ModelCache
{
	// The left-hand value of the ModelCache map
	typedef std::pair<std::string, std::string> ModelKey;
	
	typedef std::map<ModelKey, scene::INodePtr> ModelNodeMap;
	ModelNodeMap _modelNodeMap;

	// Flag to disable the cache on demand (used during clear())
	bool _enabled;
	
public:
	ModelCache();
	
	/**
	 * greebo: Looks up the model with the given path/name in the cache.
	 * 
	 * @returns: The singleton model NodePtr or NULL if not found.
	 */
	virtual scene::INodePtr find(const std::string& path, const std::string& name);
	
	/**
	 * greebo: Inserts the given node into the modelcache. 
	 */
	virtual void insert(const std::string& path, const std::string& name, const scene::INodePtr& modelNode);
	
	/**
	 * greebo: Removes the given path from the cache.
	 */
	virtual void erase(const std::string& path, const std::string& name);
	
	// Clears the cache
	virtual void clear();
	
	static ModelCache& Instance();
};

} // namespace model

#endif /*MODELCACHE_H_*/
