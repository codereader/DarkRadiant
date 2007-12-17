#ifndef MODELCACHE_H_
#define MODELCACHE_H_

#include <map>
#include <string>
#include "inode.h"

namespace model {

class ModelCache
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
	
	static ModelCache& Instance();
};

} // namespace model

#endif /*MODELCACHE_H_*/
