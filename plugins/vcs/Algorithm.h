#pragma once

#include "imap.h"
#include "i18n.h"
#include "itextstream.h"
#include "Repository.h"
#include "GitException.h"
#include "Diff.h"
#include <git2.h>

namespace vcs
{

namespace git
{

enum class RequiredMergeStrategy
{
    // The local branch is up to date
    NoMergeRequired,

    // Only the local branch got commits they can be pushed
    JustPush,

    // The local branch can be fast-forwarded since there are no local changes
    FastForward,

    // At most one branch changed the map, the branches can be merged
    MergeRecursively,

    // Both branches changed the loaded map, a merge is required
    MergeMap,

    // The local map has uncommitted changes, and the remote is trying to change it
    MergeMapWithUncommittedChanges,
};

struct RemoteStatus
{
    std::size_t localAheadCount;
    std::size_t remoteAheadCount;
    std::string label;

    RequiredMergeStrategy strategy;
};

inline RemoteStatus analyseRemoteStatus(const std::shared_ptr<Repository>& repository)
{
    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());

    if (mapPath.empty())
    {
        return RemoteStatus{ 0, 0, _("-") };
    }

    auto status = repository->getSyncStatusOfBranch(*repository->getHead());
    auto mapFileHasUncommittedChanges = repository->fileHasUncommittedChanges(mapPath);

    if (status.remoteCommitsAhead == 0)
    {
        return status.localCommitsAhead == 0 ?
            RemoteStatus{ status.localCommitsAhead, 0, _("Up to date"), RequiredMergeStrategy::NoMergeRequired } :
            RemoteStatus{ status.localCommitsAhead, 0, _("Pending Upload"), RequiredMergeStrategy::JustPush };
    }
    else if (status.localCommitsAhead == 0 && !mapFileHasUncommittedChanges)
    {
        // No local commits and no uncommitted changes, we can fast-forward
        return RemoteStatus{ 0, status.remoteCommitsAhead, _("Integrate"), RequiredMergeStrategy::FastForward };
    }

    // Check the incoming commits for modifications of the loaded map
    auto head = repository->getHead();
    auto upstream = head->getUpstream();

    // Find the merge base for this ref and its upstream
    auto mergeBase = repository->findMergeBase(*head, *upstream);

    auto remoteDiffAgainstBase = repository->getDiff(*upstream, *mergeBase);
    
    bool remoteDiffContainsMap = remoteDiffAgainstBase->containsFile(mapPath);

    if (!remoteDiffContainsMap)
    {
        // Remote didn't change the map, we can integrate it without conflicting the loaded map
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Integrate"), 
            RequiredMergeStrategy::MergeRecursively };
    }

    if (mapFileHasUncommittedChanges)
    {
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Commit, then integrate "),
            RequiredMergeStrategy::MergeMapWithUncommittedChanges };
    }

    auto localDiffAgainstBase = repository->getDiff(*head, *mergeBase);
    bool localDiffContainsMap = localDiffAgainstBase->containsFile(mapPath);

    if (!localDiffContainsMap)
    {
        // The local diff doesn't include the map, the remote changes can be integrated
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Integrate"),
            RequiredMergeStrategy::MergeRecursively };
    }

    // Both the local and the remote diff are affecting the map file, this needs resolution
    return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, _("Resolve"),
        RequiredMergeStrategy::MergeMap };
}

inline void syncWithRemote(const std::shared_ptr<Repository>& repository)
{
    if (GlobalMapModule().isModified())
    {
        throw git::GitException(_("The map file has unsaved changes, please save before merging."));
    }

    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());
    /*auto mapFileHasUncommittedChanges = !mapPath.empty() && repository->fileHasUncommittedChanges(mapPath);

    if (mapFileHasUncommittedChanges)
    {
        throw git::GitException(_("The map file has uncommitted changes, cannot merge yet."));
    }*/

    RemoteStatus status = analyseRemoteStatus(repository);

    switch (status.strategy)
    {
    case RequiredMergeStrategy::NoMergeRequired:
        rMessage() << "No merge required." << std::endl;
        return;

    case RequiredMergeStrategy::JustPush:
        repository->pushToTrackedRemote();
        return;

    case RequiredMergeStrategy::FastForward:
        repository->fastForwardToTrackedRemote();
        return;
    }

    // All the other merge strategies include a recursive merge, prepare it
    if (status.strategy == RequiredMergeStrategy::MergeMapWithUncommittedChanges)
    {
        // Commit the current map changes
        // TODO
    }

    if (!repository->isReadyForMerge())
    {
        throw git::GitException(_("Repository is not ready for a merge at this point"));
    }

    if (!repository->getHead() || !repository->getHead()->getUpstream())
    {
        throw git::GitException(_("Cannot resolve HEAD and the corresponding upstream branch"));
    }

    git_merge_analysis_t analysis;
    git_merge_preference_t preference = GIT_MERGE_PREFERENCE_NONE;

    git_annotated_commit* mergeHead;

    auto upstream = repository->getHead()->getUpstream();
    git_oid upstreamOid;
    auto error = git_reference_name_to_id(&upstreamOid, repository->_get(), upstream->getName().c_str());

    error = git_annotated_commit_lookup(&mergeHead, repository->_get(), &upstreamOid);
    GitException::ThrowOnError(error);

    try
    {
        std::vector<const git_annotated_commit*> mergeHeads;
        mergeHeads.push_back(mergeHead);

        error = git_merge_analysis(&analysis, &preference, repository->_get(), mergeHeads.data(), mergeHeads.size());
        GitException::ThrowOnError(error);

        // The result of the analysis must be that a three-way merge is required
        if (analysis != GIT_MERGE_ANALYSIS_NORMAL)
        {
            throw GitException("The repository state doesn't require a regular merge, cannot proceed.");
        }
        
        git_merge_options mergeOptions = GIT_MERGE_OPTIONS_INIT;
        git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_ALLOW_CONFLICTS;

        error = git_merge(repository->_get(), mergeHeads.data(), mergeHeads.size(), &mergeOptions, &checkoutOptions);
        GitException::ThrowOnError(error);
        
        // At this point, check if the loaded map is affected by the merge
        if (status.strategy == RequiredMergeStrategy::MergeMap)
        {
            // The loaded map merge needs to be confirmed by the user

        }

        auto index = repository->getIndex();

        if (index->hasConflicts())
        {
            // Handle conflicts -notify user?
        }
        else
        {
            //create_merge_commit(repository->_get(), index, &mergeOptions);
        }

        git_repository_state_cleanup(repository->_get());
    }
    catch (const GitException& ex)
    {
        git_annotated_commit_free(mergeHead);
        throw ex;
    }
}

}

}
