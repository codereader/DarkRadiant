#include "VersionControlManager.h"

#include "itextstream.h"
#include "module/StaticModule.h"

namespace vcs
{

void VersionControlManager::registerModule(const IVersionControlModule::Ptr& vcsModule)
{
    auto result = _registeredModules.emplace(vcsModule->getUriPrefix(), vcsModule);

    if (!result.second)
    {
        throw std::runtime_error("A VCS module with prefix " + vcsModule->getUriPrefix() + " has already been registered.");
    }
}

void VersionControlManager::unregisterModule(const IVersionControlModule::Ptr& vcsModule)
{
    _registeredModules.erase(vcsModule->getUriPrefix());
}

IVersionControlModule::Ptr VersionControlManager::getModuleForPrefix(const std::string& prefix)
{
    auto existing = _registeredModules.find(prefix);

    return existing != _registeredModules.end() ? existing->second : IVersionControlModule::Ptr();
}

const std::string& VersionControlManager::getName() const
{
    static std::string _name(vcs::MODULE_VERSION_CONTROL_MANAGER);
    return _name;
}

const StringSet& VersionControlManager::getDependencies() const
{
    static StringSet _dependencies;
    return _dependencies;
}

void VersionControlManager::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;
}

module::StaticModuleRegistration<VersionControlManager> versionControlManagerModule;

}
