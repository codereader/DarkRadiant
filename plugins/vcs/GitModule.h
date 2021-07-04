#pragma once

#include "imodule.h"

namespace vcs
{

class GitModule :
    public RegisterableModule
{
public:
    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;
};

}
