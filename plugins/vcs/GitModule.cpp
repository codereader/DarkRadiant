#include "GitModule.h"

#include "igame.h"
#include "git2/repository.h"

namespace vcs
{

const std::string& GitModule::getName() const
{
    static std::string _name("GitIntegration");
    return _name;
}

const StringSet& GitModule::getDependencies() const
{
    static StringSet _dependencies{};
    return _dependencies;
}

void GitModule::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    auto modPath = GlobalGameManager().getModPath();
    git_repository* repository;
    
    if (git_repository_open(&repository, modPath.c_str()) == 0)
    {
        rMessage() << "Opened repository at " << modPath << std::endl;
    }
    else
    {
        rMessage() << "Failed to open repository at " << modPath << std::endl;
    }
}

void GitModule::shutdownModule()
{
    rMessage() << getName() << "::shutdownModule called." << std::endl;


}


}
