#include "SpacePartitionSystemFactory.h"

#include "irender.h"
#include "itextstream.h"
#include "modulesystem/StaticModule.h"

#include "Octree.h"

namespace scene
{

const std::string& SpacePartitionSystemFactory::getName() const
{
	static std::string _name(MODULE_SPACE_PARTITION_FACTORY);
	return _name;
}

const StringSet& SpacePartitionSystemFactory::getDependencies() const
{
	static StringSet _dependencies; 

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_RENDERSYSTEM);
	}

	return _dependencies;
}

void SpacePartitionSystemFactory::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "SpacePartitionSystemFactory::initialiseModule called" << std::endl;
}

ISpacePartitionSystemPtr SpacePartitionSystemFactory::create()
{
	return ISpacePartitionSystemPtr(new Octree);
}

// Define a static SpacePartitionSystemFactory module 
module::StaticModule<SpacePartitionSystemFactory> staticSpacePartitionSystemFactoryModule;

} // namespace scene
