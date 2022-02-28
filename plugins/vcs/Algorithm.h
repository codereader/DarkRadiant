#pragma once

#include "imap.h"
#include "i18n.h"
#include "itextstream.h"
#include "Repository.h"
#include "gamelib.h"
#include "GitException.h"
#include "GitModule.h"
#include "Commit.h"
#include "Diff.h"
#include "VersionControlLib.h"
#include "command/ExecutionFailure.h"
#include "wxutil/dialog/MessageBox.h"
#include "ui/CommitDialog.h"
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

    // Merge is not possible at this point
    MergeAlreadyInProgress,
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

    if (mapPath.empty() || !repository->getHead())
    {
        return RemoteStatus{ 0, 0, _("-") };
    }

    if (!repository->getHead()->getUpstream())
    {
        return RemoteStatus{ 0, 0, _("No tracked remote"), RequiredMergeStrategy::NoMergeRequired };
    }

    auto status = repository->getSyncStatusOfBranch(*repository->getHead());

    if (repository->mergeIsInProgress())
    {
        return RemoteStatus{ status.localCommitsAhead, status.remoteCommitsAhead, 
            _("Merge in progress"), RequiredMergeStrategy::MergeAlreadyInProgress };
    }

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
            status.localCommitsAhead == 0 ? RequiredMergeStrategy::FastForward : RequiredMergeStrategy::MergeRecursively };
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

inline std::string getInfoFilePath(const std::string& mapPath)
{
    auto format = GlobalMapFormatManager().getMapFormatForFilename(mapPath);

    if (format && format->allowInfoFileCreation())
    {
        return os::replaceExtension(mapPath, game::current::getInfoFileExtension());
    }

    return std::string();
}

inline void resolveMapFileConflictUsingOurs(const std::shared_ptr<Repository>& repository)
{
    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());
    auto index = repository->getIndex();

    // Remove the conflict state of the map file after save
    if (!mapPath.empty() && index->fileIsConflicted(mapPath))
    {
        index->resolveByUsingOurs(mapPath);

        auto infoFilePath = getInfoFilePath(mapPath);

        if (!infoFilePath.empty())
        {
            index->resolveByUsingOurs(infoFilePath);
        }

        index->write();
    }
}

inline void tryToFinishMerge(const std::shared_ptr<Repository>& repository)
{
    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());

    auto index = repository->getIndex();

    if (index->hasConflicts())
    {
        // Remove the conflict state from the map
        resolveMapFileConflictUsingOurs(repository);
        
        // If the index still has conflicts, notify the user
        if (index->hasConflicts())
        {
            wxutil::Messagebox::Show(_("Conflicts"),
                _("There are still unresolved conflicts in the repository.\nPlease use your Git client to resolve them and try again."),
                ::ui::IDialog::MessageType::MESSAGE_CONFIRM);
            return;
        }
    }

    auto head = repository->getHead();
    if (!head) throw git::GitException("Cannot resolve repository HEAD");

    auto upstream = head->getUpstream();
    if (!upstream) throw git::GitException("Cannot resolve upstream ref from HEAD");

    // We need the commit metadata to be valid
    git::CommitMetadata metadata;

    metadata.name = repository->getConfigValue("user.name");
    metadata.email = repository->getConfigValue("user.email");
    metadata.message = "Integrated remote changes from " + upstream->getShorthandName();

    if (metadata.name.empty() || metadata.email.empty())
    {
        metadata = ui::CommitDialog::RunDialog(metadata);
    }

    if (metadata.isValid())
    {
        repository->createCommit(metadata, upstream);
        repository->cleanupState();
    }
}

inline void performFastForward(const std::shared_ptr<Repository>& repository)
{
    auto head = repository->getHead();
    auto upstream = head->getUpstream();

    // Find the merge base for this ref and its upstream
    auto mergeBase = repository->findMergeBase(*head, *upstream);

    auto remoteDiffAgainstBase = repository->getDiff(*upstream, *mergeBase);

    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());
    bool remoteDiffContainsMap = remoteDiffAgainstBase->containsFile(mapPath);

    repository->fastForwardToTrackedRemote();

    if (!remoteDiffContainsMap)
    {
        return;
    }

    // The map has been modified on disk, so it might be a good choice to reload the map
    if (wxutil::Messagebox::Show(_("Map has been updated"),
        _("The map file has been updated on disk, reload the map file now?"),
        ::ui::IDialog::MessageType::MESSAGE_ASK) == ::ui::IDialog::RESULT_YES)
    {
        GlobalCommandSystem().executeCommand("OpenMap", GlobalMapModule().getMapName());
    }
}

inline void syncWithRemote(const std::shared_ptr<Repository>& repository)
{
    if (repository->mergeIsInProgress())
    {
        throw GitException(_("Merge in progress"));
    }

    if (GlobalMapModule().isModified())
    {
        throw GitException(_("The map file has unsaved changes, please save before merging."));
    }

    auto mapPath = repository->getRepositoryRelativePath(GlobalMapModule().getMapName());
    
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
        performFastForward(repository);
        return;
    }

    // All the other merge strategies include a recursive merge, prepare it
    if (status.strategy == RequiredMergeStrategy::MergeMapWithUncommittedChanges)
    {
        // Commit the current map changes
        wxutil::Messagebox::Show(_("Pending Commit"),
            _("The map file has uncommitted changes, please commit first before integrating."),
            ::ui::IDialog::MessageType::MESSAGE_CONFIRM);
        return;
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

    auto upstream = repository->getHead()->getUpstream();
    git_oid upstreamOid;
    auto error = git_reference_name_to_id(&upstreamOid, repository->_get(), upstream->getName().c_str());

    git_annotated_commit* mergeHead;
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
        
        // Check if the loaded map is affected by the merge
        if (status.strategy != RequiredMergeStrategy::MergeMap)
        {
            tryToFinishMerge(repository);
            return;
        }

        // A map merge is required
        wxutil::Messagebox::Show(_("Map Merge Required"),
            _("The map has been changed both on the server and locally.\n"
                "DarkRadiant will now switch to Merge Mode to highlight the differences.\n"
                "Please have a look, resolve possible conflicts and finish the merge."),
            ::ui::IDialog::MessageType::MESSAGE_CONFIRM);

        auto mergeBase = repository->findMergeBase(*repository->getHead(), *upstream);

        auto baseUri = constructVcsFileUri(GitModule::UriPrefix, Reference::OidToString(mergeBase->getOid()), mapPath);
        auto sourceUri = constructVcsFileUri(GitModule::UriPrefix, Reference::OidToString(&upstreamOid), mapPath);

        try
        {
            // The loaded map merge needs to be confirmed by the user
            GlobalMapModule().startMergeOperation(sourceUri, baseUri);
            // Done here, wait until the user finishes or aborts the merge
            return;
        }
        catch (const cmd::ExecutionFailure& ex)
        {
            throw GitException(ex.what());
        }
    }
    catch (const GitException& ex)
    {
        git_annotated_commit_free(mergeHead);
        throw ex;
    }
}

}

}
