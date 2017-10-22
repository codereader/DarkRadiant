#pragma once

#include "imodule.h"
#include "ModuleRegistry.h"

/**
 * greebo: Use this class to define a static RegisterableModule.
 *
 * The template parameter must be a RegisterModule class and
 * is automatically registered with the ModuleRegistry by
 * the StaticModule constructor.
 *
 * If immediate registering is not desired, the constructor
 * could add the incoming modules to a static std::list
 * and another static routine would add the modules on demand.
 *
 * Note: does NOT hold actual shared_ptrs of the RegisterableModule.
 *
 * Usage: StaticModule<RegisterableModule> myStaticModule;
 */
namespace module
{

template <class ModuleType>
class StaticModule
{
	// Define a std::shared_ptr for the given class type
	typedef std::shared_ptr<ModuleType> ModuleTypePtr;
	std::string _moduleName;

public:
	// The constructor
	StaticModule()
    {
		ModuleTypePtr module(new ModuleType());
		_moduleName = module->getName();
		ModuleRegistry::Instance().registerModule(module);
	}

	inline ModuleTypePtr getModule()
    {
		return std::static_pointer_cast<ModuleType>(
			ModuleRegistry::Instance().getModule(_moduleName)
		);
	}
};

} // namespace module

