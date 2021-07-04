#include "GitModule.h"

#include "igame.h"
#include "imainframe.h"
#include "istatusbarmanager.h"
#include "icommandsystem.h"
#include <git2.h>
#include "Repository.h"
#include "Remote.h"
#include "ui/VcsStatus.h"

namespace vcs
{

const std::string& GitModule::getName() const
{
    static std::string _name("GitIntegration");
    return _name;
}

const StringSet& GitModule::getDependencies() const
{
    static StringSet _dependencies{ MODULE_MAINFRAME, MODULE_STATUSBARMANAGER };
    return _dependencies;
}

void GitModule::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    // Register commands
    registerCommands();

    // Initialise libgit2 to have the memory handlers etc. set up
    git_libgit2_init();

    auto modPath = GlobalGameManager().getModPath();
    _repository = std::make_unique<git::Repository>(modPath);
    
    if (_repository->isOk())
    {
        rMessage() << "Opened repository at " << modPath << std::endl;
        rMessage() << "Repository is currently on branch " << _repository->getCurrentBranchName() << std::endl;
    }
    else
    {
        _repository.reset();
    }

#if 0
        git_commit* commit;
        git_oid oid;
        git_reference_name_to_id(&oid, repository, "refs/heads/master");

        git_commit_lookup(&commit, repository, &oid);

        const auto* author = git_commit_author(commit);
        auto time = git_commit_time(commit);
        rMessage() << "Last commit author: " << author->name << " at " << ctime(&time) << std::endl;

        git_commit_free(commit);
#endif

    GlobalMainFrame().signal_MainFrameConstructed().connect([&]()
    {
        GlobalStatusBarManager().addElement(ui::VcsStatus::Name, new ui::VcsStatus(GlobalStatusBarManager().getStatusBar()), 
            ::ui::statusbar::StandardPosition::MapEditStopwatch + 10);
    });
}

void GitModule::shutdownModule()
{
    rMessage() << getName() << "::shutdownModule called." << std::endl;

    _repository.reset();

    git_libgit2_shutdown();
}

void GitModule::registerCommands()
{
    GlobalCommandSystem().addCommand("GitFetch", std::bind(&GitModule::fetch, this, std::placeholders::_1));
}

void GitModule::fetch(const cmd::ArgumentList& args)
{
    if (!_repository)
    {
        rWarning() << "Project is not under version control" << std::endl;
        return;
    }

    auto head = _repository->getHead();

    if (!head)
    {
        rWarning() << "Could not retrieve HEAD reference from repository" << std::endl;
        return;
    }

    auto trackedBranch = head->getUpstream();

    rMessage() << head->getShorthandName() << " is set up to track " << (trackedBranch ? trackedBranch->getShorthandName() : "-") << std::endl;

    auto remoteName = _repository->getUpstreamRemoteName(*head);
    rMessage() << head->getShorthandName() << " is set up to track remote " << remoteName << std::endl;

    auto remote = _repository->getRemote(remoteName);

    if (!remote)
    {
        rWarning() << "Cannot fetch from remote 'origin'" << std::endl;
        return;
    }

    remote->fetch();
}

}

/**
 * greebo: This is the module entry point which the main binary will look for.
 * The symbol RegisterModule is called with the singleton ModuleRegistry as argument.
 */
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry & registry)
{
    module::performDefaultInitialisation(registry);

    registry.registerModule(std::make_shared<vcs::GitModule>());
}
