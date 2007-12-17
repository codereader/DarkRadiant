#ifndef IMODELCACHE_H_
#define IMODELCACHE_H_

#include "imodule.h"
#include "inode.h"

namespace model {

class IModelCache :
	public RegisterableModule
{
public:
	
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
