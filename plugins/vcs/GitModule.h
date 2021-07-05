#pragma once

#include "imodule.h"
#include "icommandsystem.h"
#include "Repository.h"

namespace vcs
{

namespace ui { class VcsStatus;  }

class GitModule :
    public RegisterableModule
{
private:
    std::shared_ptr<git::Repository> _repository;

    ui::VcsStatus* _statusBarWidget = nullptr;

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
