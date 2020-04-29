#pragma once

#include "imodule.h"
#include "ModuleRegistry.h"

/**
 * greebo: Use a StaticModule to define a static RegisterableModule,
 * which enlists itself automatically during application startup.
 *
 * The template parameter must be a RegisterModule class and
 * is automatically registered with the ModuleRegistry by
 * the StaticModule constructor.
 *
 * Since immediate registering is not desired, the constructor
 * adds the incoming modules to a static std::list
 * for a later routine to add the modules to the registry.
 *
 * Note: does NOT hold actual shared_ptrs of the RegisterableModule.
 *
 * Usage: StaticModule<RegisterableModule> myStaticModule;
 */
namespace module
{

namespace internal
{

/**
 * Static container holding the modules registered by
 * the StaticModule<T> helper. They will be picked up
 * and acquired by the ModuleRegistry, after which point
 * this list will be cleared.
 */
class StaticModuleList :
    private std::list<RegisterableModulePtr>
{
public:
    ~StaticModuleList();

    static void Add(const RegisterableModulePtr& module);

    static void ForEachModule(const std::function<void(const RegisterableModulePtr&)>& func);

    static void Clear();

private:
    static StaticModuleList& Instance();
};

}

template <class ModuleType>
class StaticModule
{
    static_assert(std::is_base_of<RegisterableModule, ModuleType>::value, "ModuleType must be of type RegisterableModule");

private:
	std::string _moduleName;

public:
	StaticModule()
    {
        auto module = std::make_shared<ModuleType>();
		_moduleName = module->getName();

        // Add this to the list in the backend, it will be picked up later
        internal::StaticModuleList::Add(module);
	}

	inline std::shared_ptr<ModuleType> getModule()
    {
		return std::static_pointer_cast<ModuleType>(
            GlobalModuleRegistry().getModule(_moduleName)
		);
	}
};

} // namespace module

