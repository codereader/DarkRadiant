#include "Repository.h"

#include "i18n.h"
#include <git2.h>
#include "itextstream.h"
#include "imap.h"
#include "Remote.h"
#include "Commit.h"
#include "Tree.h"
#include "Diff.h"
#include "GitException.h"
#include "os/path.h"
#include "os/file.h"
#include "fmt/format.h"

namespace vcs
{

namespace git
{

Repository::Repository(const std::string& path) :
    _repository(nullptr),
    _isOk(false),
    _path(os::standardPathWithSlash(path))
{
    if (git_repository_open(&_repository, _path.c_str()) == 0)
    {
        _isOk = true;
    }
    else
    {
        rMessage() << "Failed to open repository at " << _path << std::endl;
    }
}

Repository::~Repository()
{
    git_repository_free(_repository);
}

bool Repository::isOk() const
{
    return _isOk;
}

const std::string& Repository::getPath() const
{
    return _path;
}

std::string Repository::getRepositoryRelativePath(const std::string& path)
{
    if (!os::fileOrDirExists(path))
    {
        return ""; // doesn't exist
    }

    auto relativePath = os::getRelativePath(path, getPath());

    if (relativePath == path)
    {
        return ""; // outside VCS
    }

    return relativePath;
}

std::shared_ptr<Repository> Repository::clone()
{
    return std::make_shared<Repository>(_path);
}

std::shared_ptr<Remote> Repository::getRemote(const std::string& name)
{
    return Remote::CreateFromName(*this, name);
}

Reference::Ptr Repository::getHead()
{
    git_reference* head;
    int error = git_repository_head(&head, _repository);

    if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
    {
        return Reference::Ptr();
    }

    return std::make_shared<Reference>(head);
}

std::string Repository::getCurrentBranchName()
{
    auto head = getHead();
    return head ? head->getShorthandName() : std::string();
}

std::string Repository::getUpstreamRemoteName(const Reference& reference)
{
    git_buf buf;
    memset(&buf, 0, sizeof(git_buf));

    auto error = git_branch_upstream_remote(&buf, _repository, reference.getName().c_str());
    GitException::ThrowOnError(error);

    std::string upstreamRemote = buf.ptr;
    git_buf_dispose(&buf);

    return upstreamRemote;
}

Remote::Ptr Repository::getTrackedRemote()
{
    auto head = getHead();

    if (!head)
    {
        throw GitException(_("Could not retrieve HEAD reference from repository"));
    }

    auto trackedBranch = head->getUpstream();

    rMessage() << head->getShorthandName() << " is set up to track " << (trackedBranch ? trackedBranch->getShorthandName() : "-") << std::endl;

    if (!trackedBranch)
    {
        throw GitException(_("No tracked remote branch configured"));
    }

    auto remoteName = getUpstreamRemoteName(*head);
    rMessage() << head->getShorthandName() << " is set up to track remote " << remoteName << std::endl;

    auto remote = getRemote(remoteName);

    if (!remote)
    {
        throw GitException(fmt::format(_("Failed to get the named remote: {0}"), remoteName));
    }

    return remote;
}

void Repository::fetchFromTrackedRemote()
{
    auto remote = getTrackedRemote();
    remote->fetch();
}

void Repository::pushToTrackedRemote()
{
    auto remote = getTrackedRemote();
    remote->push(*getHead()); // getHead will succeed because getTrackedRemote did
}

void Repository::fastForwardToTrackedRemote()
{
    auto head = getHead();
    if (!head) throw GitException(_("Could not retrieve HEAD reference from repository"));

    auto upstream = head->getUpstream();
    if (!upstream) throw GitException(_("No tracked remote branch configured"));

    // Lookup the target object
    git_oid targetOid;
    git_reference_name_to_id(&targetOid, _repository, upstream->getName().c_str());

    git_object* target;
    auto error = git_object_lookup(&target, _repository, &targetOid, GIT_OBJECT_COMMIT);
    GitException::ThrowOnError(error);

    rMessage() << "Fast-fowarding " << head->getName() << " to upstream " << upstream->getName() << std::endl;

    try
    {
        // Checkout the result so the workdir is in the expected state
        git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOptions.checkout_strategy = GIT_CHECKOUT_SAFE;

        error = git_checkout_tree(_repository, target, &checkoutOptions);
        GitException::ThrowOnError(error);
        
        // Move the target reference to the target OID
        head->setTarget(&targetOid);

        rMessage() << "Fast-foward done, " << head->getName() << " is now at " << Reference::OidToString(&targetOid) << std::endl;
    }
    catch (const GitException& ex)
    {
        git_object_free(target);
        throw ex;
    }
}

RefSyncStatus Repository::getSyncStatusOfBranch(const Reference& reference)
{
    RefSyncStatus status;

    auto trackedBranch = reference.getUpstream();

    if (!trackedBranch) throw GitException(_("The current branch doesn't track a remote, cannot check sync status"));

    git_revwalk* walker;
    git_revwalk_new(&walker, _repository);

    // Start from remote
    git_revwalk_push_ref(walker, trackedBranch->getName().c_str());

    // End at local
    git_oid refOid;
    git_reference_name_to_id(&refOid, _repository, reference.getName().c_str());
    git_revwalk_hide(walker, &refOid);

    git_oid id;
    while (!git_revwalk_next(&id, walker))
    {
        //rMessage() << Reference::OidToString(&id) << " => ";
        ++status.remoteCommitsAhead;
    }
    //rMessage() << std::endl;

    git_revwalk_free(walker);

    // Another walk from local to remote
    git_revwalk_new(&walker, _repository);

    git_revwalk_push(walker, &refOid);
    git_revwalk_hide_ref(walker, trackedBranch->getName().c_str());

    while (!git_revwalk_next(&id, walker))
    {
        //rMessage() << Reference::OidToString(&id) << " => ";
        ++status.localCommitsAhead;
    }
    //rMessage() << std::endl;

    git_revwalk_free(walker);

    // Initialise the convenience flags
    status.localIsUpToDate = status.localCommitsAhead == 0 && status.remoteCommitsAhead == 0;
    status.localCanBePushed = status.localCommitsAhead > 0 && status.remoteCommitsAhead == 0;

    return status;
}

bool Repository::isUpToDateWithRemote()
{
    auto head = getHead();

    if (!head)
    {
        rWarning() << "Could not retrieve HEAD reference from repository" << std::endl;
        return false;
    }

    return getSyncStatusOfBranch(*head).localIsUpToDate;
}

unsigned int Repository::getFileStatus(const std::string& relativePath)
{
    git_status_options options = GIT_STATUS_OPTIONS_INIT;

    char* paths[] = { const_cast<char*>(relativePath.c_str()) };

    options.pathspec.count = 1;
    options.pathspec.strings = paths;
    options.flags |= GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    options.show = GIT_STATUS_SHOW_WORKDIR_ONLY;

    unsigned int statusFlags = 0;

    auto error = git_status_foreach_ext(_repository, &options, [](const char* path, unsigned int flags, void* payload)
    {
        *reinterpret_cast<unsigned int*>(payload) = flags;
        return 0;
    }, &statusFlags);
    GitException::ThrowOnError(error);

    return statusFlags;
}

bool Repository::fileIsIndexed(const std::string& relativePath)
{
    return (getFileStatus(relativePath) & GIT_STATUS_WT_NEW) == 0;
}

bool Repository::fileHasUncommittedChanges(const std::string& relativePath)
{
    return (getFileStatus(relativePath) & GIT_STATUS_WT_MODIFIED) != 0;
}

Index::Ptr Repository::getIndex()
{
    git_index* index;
    auto error = git_repository_index(&index, _repository);
    GitException::ThrowOnError(error);

    return std::make_shared<Index>(index);
}

std::shared_ptr<Tree> Repository::getTreeByRevision(const std::string& revision)
{
    git_oid revisionOid;
    auto error = git_oid_fromstr(&revisionOid, revision.c_str());
    GitException::ThrowOnError(error);

    auto commit = Commit::LookupFromOid(_repository, &revisionOid);
    return commit->getTree();
}

void Repository::createCommit(const CommitMetadata& metadata)
{
    createCommit(metadata, Reference::Ptr());
}

void Repository::createCommit(const CommitMetadata& metadata, const Reference::Ptr& additionalParent)
{
    auto head = getHead();
    auto index = getIndex();

    rMessage() << "Creating commit with user " << metadata.name << std::endl;

    git_signature* signature;
    auto error = git_signature_now(&signature, metadata.name.c_str(), metadata.email.c_str());
    GitException::ThrowOnError(error);

    // Add all working copy changes
    index->updateAll();

    auto tree = index->writeTree(*this);

    std::vector<const git_commit*> parentCommits;

    // It's possible that there is no HEAD yet (first commit in the repo)
    if (head)
    {
        git_oid headOid;
        error = git_reference_name_to_id(&headOid, _repository, head->getName().c_str());
        GitException::ThrowOnError(error);
        
        auto parentCommit = Commit::LookupFromOid(_repository, &headOid);
        parentCommits.push_back(parentCommit->_get());
    }
    
    // Check if we have an additional parent
    if (additionalParent)
    {
        git_oid parentOid;
        auto error = git_reference_name_to_id(&parentOid, _repository, additionalParent->getName().c_str());
        GitException::ThrowOnError(error);

        auto additionalParentCommit = Commit::LookupFromOid(_repository, &parentOid);

        parentCommits.push_back(additionalParentCommit->_get());
    }

    git_oid commitOid;
    error = git_commit_create(&commitOid,
        _repository, head ? head->getName().c_str() : "HEAD",
        signature, signature,
        nullptr, metadata.message.c_str(),
        tree->_get(),
        parentCommits.size(), parentCommits.data());
    GitException::ThrowOnError(error);

    index->write();

    rMessage() << "Commit created: " << Reference::OidToString(&commitOid) << std::endl;
}

std::string Repository::getConfigValue(const std::string& key)
{
    git_config* config;
    auto error = git_repository_config_snapshot(&config, _repository);
    GitException::ThrowOnError(error);

    try
    {
        const char* value;
        auto error = git_config_get_string(&value, config, key.c_str());
        GitException::ThrowOnError(error);

        // Copy the value before free-ing the config
        std::string returnValue(value);

        git_config_free(config);

        return returnValue;
    }
    catch (const GitException& ex)
    {
        git_config_free(config);
        throw ex;
    }
}

void Repository::cleanupState()
{
    auto error = git_repository_state_cleanup(_repository);
    GitException::ThrowOnError(error);
}

bool Repository::isReadyForMerge()
{
    auto state = git_repository_state(_repository);
    return state == GIT_REPOSITORY_STATE_NONE;
}

bool Repository::mergeIsInProgress()
{
    auto state = git_repository_state(_repository);
    return state == GIT_REPOSITORY_STATE_MERGE;
}

void Repository::abortMerge()
{
    if (!mergeIsInProgress())
    {
        return;
    }

    auto head = getHead();

    git_oid targetOid;
    auto error = git_reference_name_to_id(&targetOid, _repository, head->getName().c_str());
    GitException::ThrowOnError(error);

    git_object* target;
    error = git_object_lookup(&target, _repository, &targetOid, GIT_OBJECT_COMMIT);
    GitException::ThrowOnError(error);

    git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
    checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE;

    error = git_reset(_repository, target, GIT_RESET_HARD, &checkoutOptions);
    GitException::ThrowOnError(error);
}

Commit::Ptr Repository::findMergeBase(const Reference& first, const Reference& second)
{
    git_oid firstOid;
    auto error = git_reference_name_to_id(&firstOid, _repository, first.getName().c_str());
    GitException::ThrowOnError(error);

    git_oid secondOid;
    error = git_reference_name_to_id(&secondOid, _repository, second.getName().c_str());
    GitException::ThrowOnError(error);

    git_oid mergeBase;
    error = git_merge_base(&mergeBase, _repository, &firstOid, &secondOid);
    GitException::ThrowOnError(error);

    git_commit* commit;
    error = git_commit_lookup(&commit, _repository, &mergeBase);
    GitException::ThrowOnError(error);

    return std::make_shared<Commit>(commit);
}

std::shared_ptr<Diff> Repository::getDiff(const Reference& ref, Commit& commit)
{
    git_oid refOid;
    auto error = git_reference_name_to_id(&refOid, _repository, ref.getName().c_str());
    GitException::ThrowOnError(error);

    auto refCommit = Commit::LookupFromOid(_repository, &refOid);
    auto refTree = refCommit->getTree();

    git_diff* diff;
    auto baseTree = commit.getTree();
    error = git_diff_tree_to_tree(&diff, _repository, baseTree->_get(), refTree->_get(), nullptr);
    GitException::ThrowOnError(error);

    return std::make_shared<Diff>(diff);
}

git_repository* Repository::_get()
{
    return _repository;
}

}

}
