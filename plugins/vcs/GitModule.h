#pragma once

#include "imodule.h"
#include "icommandsystem.h"
#include "Repository.h"

namespace vcs
{

namespace ui { class VcsStatus;  }

const char* const RKEY_AUTO_FETCH_ENABLED = "user/ui/vcs/git/autoFetchEnabled";
const char* const RKEY_AUTO_FETCH_INTERVAL = "user/ui/vcs/git/autoFetchInterval";

class GitModule :
    public RegisterableModule
{
private:
    std::shared_ptr<git::Repository> _repository;

    std::unique_ptr<ui::VcsStatus> _statusBarWidget;

public:
    // RegisterableModule implementation
    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void registerCommands();
    void createPreferencePage();

    void fetch(const cmd::ArgumentList& args);
};

}
