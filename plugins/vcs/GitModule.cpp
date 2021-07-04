#include "GitModule.h"

#include "igame.h"
#include "git2.h"
#include "Repository.h"

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

    // Initialise libgit2 to have the memory handlers etc. set up
    git_libgit2_init();

    auto modPath = GlobalGameManager().getModPath();
    
    git::Repository repository(modPath);
    
    if (repository.isOk())
    {
        rMessage() << "Opened repository at " << modPath << std::endl;
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
}

void GitModule::shutdownModule()
{
    rMessage() << getName() << "::shutdownModule called." << std::endl;

    git_libgit2_shutdown();
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
