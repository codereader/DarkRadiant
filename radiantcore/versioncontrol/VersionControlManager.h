#pragma once

#include <map>
#include "iversioncontrol.h"

namespace vcs
{

class VersionControlManager final :
    public IVersionControlManager
{
private:
    std::map<std::string, IVersionControlModule::Ptr> _registeredModules;

public:
    void registerModule(const IVersionControlModule::Ptr& vcsModule) override;
    void unregisterModule(const IVersionControlModule::Ptr& vcsModule) override;
    IVersionControlModule::Ptr getModuleForPrefix(const std::string& prefix) override;

    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
};

}
