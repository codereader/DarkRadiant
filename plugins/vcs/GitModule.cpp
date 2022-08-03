#include "GitModule.h"

#include "i18n.h"
#include "igame.h"
#include "ui/imainframe.h"
#include "imap.h"
#include "ipreferencesystem.h"
#include "ui/istatusbarmanager.h"
#include "icommandsystem.h"
#include <git2.h>
#include "Repository.h"
#include "Commit.h"
#include "GitException.h"
#include "Remote.h"
#include "ui/VcsStatus.h"
#include "VersionControlLib.h"
#include "command/ExecutionFailure.h"

namespace vcs
{

std::string GitModule::getUriPrefix()
{
    return UriPrefix;
}

ArchiveTextFilePtr GitModule::openTextFile(const std::string& vcsUri)
{
    try
    {
        assert(getVcsPrefix(vcsUri) == UriPrefix);

        if (!_repository)
        {
            return ArchiveTextFilePtr();
        }

        auto tree = _repository->getTreeByRevision(vcs::getVcsRevision(vcsUri));
        
        return tree->openTextFile(vcs::getVcsFilePath(vcsUri), *_repository);
    }
    catch (const git::GitException& ex)
    {
        rWarning() << "git::openTextFile failed: " << ex.what() << std::endl;
        return ArchiveTextFilePtr();
    }
}

const std::string& GitModule::getName() const
{
    static std::string _name("GitIntegration");
    return _name;
}

const StringSet& GitModule::getDependencies() const
{
    static StringSet _dependencies{ MODULE_MAINFRAME, MODULE_STATUSBARMANAGER, 
        MODULE_PREFERENCESYSTEM, MODULE_MAP, MODULE_VERSION_CONTROL_MANAGER };
    return _dependencies;
}

void GitModule::initialiseModule(const IApplicationContext& ctx)
{
    // Register commands
    registerCommands();
    createPreferencePage();

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

    GlobalMainFrame().signal_MainFrameConstructed().connect([&]()
    {
        _statusBarWidget = std::make_unique<ui::VcsStatus>(GlobalStatusBarManager().getStatusBar());
        GlobalStatusBarManager().addElement(ui::VcsStatus::Name, _statusBarWidget->getWidget(),
            ::ui::statusbar::StandardPosition::MapEditStopwatch + 10);

        _statusBarWidget->setRepository(_repository);
    });

    GlobalVersionControlManager().registerModule(shared_from_this());
}

void GitModule::shutdownModule()
{
    rMessage() << getName() << "::shutdownModule called." << std::endl;

    GlobalVersionControlManager().unregisterModule(shared_from_this());

    _statusBarWidget.reset();
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

    _statusBarWidget->startFetchTask();
}

void GitModule::createPreferencePage()
{
    auto& page = GlobalPreferenceSystem().getPage(_("Settings/Version Control"));

    page.appendCheckBox(_("Enable Auto-Fetch"), RKEY_AUTO_FETCH_ENABLED);
    page.appendSpinner(_("Fetch Interval (Minutes)"), RKEY_AUTO_FETCH_INTERVAL, 0.25, 900, 2);
}

}

/**
 * greebo: This is the module entry point which the main binary will look for.
 * The symbol RegisterModule is called with the singleton ModuleRegistry as argument.
 */
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
    module::performDefaultInitialisation(registry);

    registry.registerModule(std::make_shared<vcs::GitModule>());
}
