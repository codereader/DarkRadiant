#ifndef SPACE_PARTITION_SYSTEM_FACTORY_H_
#define SPACE_PARTITION_SYSTEM_FACTORY_H_

#include "ispacepartition.h"

namespace scene
{

class SpacePartitionSystemFactory :
	public ISpacePartitionSystemFactory
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

	virtual ISpacePartitionSystemPtr create();
};

} // namespace scene

#endif /* SPACE_PARTITION_SYSTEM_FACTORY_H_ */
