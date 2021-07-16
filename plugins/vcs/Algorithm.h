#pragma once

#include "imap.h"
#include "i18n.h"
#include "Repository.h"
#include "Diff.h"

namespace vcs
{

namespace git
{

struct RemoteStatus
{
    std::size_t localAheadCount;
    std::size_t remoteAheadCount;
    std::string label;
};

inline RemoteStatus analyseRemoteStatus(const std::shared_ptr<Repository>& repository)
{
    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());

    if (mapPath.empty())
    {
        return RemoteStatus{ 0, 0, _("-") };
    }

    auto status = repository->getSyncStatusOfBranch(*repository->getHead());

    if (status.remoteCommitsAhead == 0)
    {
        return status.localCommitsAhead == 0 ?
            RemoteStatus{ status.localCommitsAhead, 0, _("Up to date") } :
            RemoteStatus{ status.localCommitsAhead, 0, _("Pending Upload") };
    }

    // Check the incoming commits for modifications of the loaded map
    auto head = repository->getHead();
    auto upstream = head->getUpstream();

    // Find the merge base for this ref and its upstream
    auto mergeBase = repository->findMergeBase(*head, *upstream);

    auto remoteDiffAgainstBase = repository->getDiff(*upstream, *mergeBase);

    if (!remoteDiffAgainstBase->containsFile(mapPath))
    {
        // Remote didn't change, we can integrate it without conflicting the loaded map
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Integrate") };
    }

    auto localDiffAgainstBase = repository->getDiff(*head, *mergeBase);

    if (repository->fileHasUncommittedChanges(mapPath))
    {
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Commit, then integrate ") };
    }

    if (!localDiffAgainstBase->containsFile(mapPath))
    {
        // The local diff doesn't include the map, the remote changes can be integrated
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Integrate") };
    }

    // Both the local and the remote diff are affecting the map file, this needs resolution
    return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Resolve") };
}

}

}
