#pragma once

#include <functional>
#include "imodule.h"

namespace module
{

namespace internal
{

/**
 * Static container holding the modules registered by
 * the StaticModuleRegistration<T> helper. They will be picked up
 * and acquired by the ModuleRegistry.
 * To support scenarios like the unit test runner (where the
 * modules are repeatedly loaded, initialised and shut down
 * again) the module list is not cleared after registration,
 * such that another startup event will have the full set of
 * modules properly registered.
 */
typedef std::function<RegisterableModulePtr()> ModuleCreationFunc;

class StaticModuleList :
    private std::list<ModuleCreationFunc>
{
public:
    static void Add(const ModuleCreationFunc& creationFunc);

    // Creates all pre-registered modules and submits them to the registry
    static void RegisterModules();

private:
    static StaticModuleList& Instance();
};

}

/**
 * @brief Helper object to register a specific RegisterableModule subclass.
 *
 * This class is intended to be used statically in the .cpp file of a particular module's
 * implementing class, to register the module implementation with the ModuleRegistry. Since
 * immediate registering is not desired, the constructor adds the incoming modules to a static
 * std::list for a later routine to add the modules to the registry.
 *
 * Note that this object does NOT hold any reference or pointer to the actual module instance. It
 * registers the TYPE of module which will later be instantiated and owned by the ModuleRegistry.
 *
 * @tparam ModuleType
 * A RegisterableModule class which is automatically registered with the ModuleRegistry by the
 * StaticModuleRegistration constructor.
 */
template <class ModuleType> class StaticModuleRegistration
{
    static_assert(std::is_base_of<RegisterableModule, ModuleType>::value,
                  "ModuleType must be of type RegisterableModule");

public:
    StaticModuleRegistration()
    {
        // Add a creator to the list in the backend, it will be called by the registry later
        internal::StaticModuleList::Add(
            []() -> RegisterableModulePtr { return std::make_shared<ModuleType>(); }
        );
    }
};

} // namespace module

