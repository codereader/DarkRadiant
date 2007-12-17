#ifndef MODELCACHE_H_
#define MODELCACHE_H_

#include "imodelcache.h"

namespace model {

class ModelCache :
	public IModelCache
{
public:
	
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

} // namespace model

#endif /*MODELCACHE_H_*/
