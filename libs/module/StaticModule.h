#pragma once

#include <functional>
#include "imodule.h"

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
typedef std::function<RegisterableModulePtr()> ModuleCreationFunc;

class StaticModuleList :
    private std::list<ModuleCreationFunc>
{
public:

    ~StaticModuleList();

    static void Add(const ModuleCreationFunc& creationFunc);

    // Creates all pre-registered modules and submits them to the registry
    static void RegisterModules();

private:
    static StaticModuleList& Instance();
};

}

template <class ModuleType>
class StaticModule
{
    static_assert(std::is_base_of<RegisterableModule, ModuleType>::value, "ModuleType must be of type RegisterableModule");

public:
	StaticModule()
    {
        // Add a creator to the list in the backend, it will be called by the registry later
        internal::StaticModuleList::Add([]()->RegisterableModulePtr
        {
            return std::make_shared<ModuleType>();
        });
	}
};

} // namespace module

