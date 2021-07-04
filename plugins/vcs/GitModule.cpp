#include "GitModule.h"

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

}

void GitModule::shutdownModule()
{
    rMessage() << getName() << "::shutdownModule called." << std::endl;


}


}
