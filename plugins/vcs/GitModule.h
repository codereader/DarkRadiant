#pragma once

#include "imodule.h"
#include "icommandsystem.h"
#include "Repository.h"

namespace vcs
{

class GitModule :
    public RegisterableModule
{
private:
    std::unique_ptr<git::Repository> _repository;

public:
    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void registerCommands();

    void fetch(const cmd::ArgumentList& args);
};

}
