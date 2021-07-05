#include "Repository.h"

#include <git2.h>
#include "itextstream.h"
#include "Remote.h"

namespace vcs
{

namespace git
{

Repository::Repository(const std::string& path) :
    _repository(nullptr),
    _isOk(false),
    _path(path)
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

bool Repository::isOk() const
{
    return _isOk;
}

Repository::~Repository()
{
    git_repository_free(_repository);
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

    git_branch_upstream_remote(&buf, _repository, reference.getName().c_str());

    std::string upstreamRemote = buf.ptr;
    git_buf_dispose(&buf);

    return upstreamRemote;
}

void Repository::fetchFromTrackedRemote()
{
    auto head = getHead();

    if (!head)
    {
        rWarning() << "Could not retrieve HEAD reference from repository" << std::endl;
        return;
    }

    auto trackedBranch = head->getUpstream();

    rMessage() << head->getShorthandName() << " is set up to track " << (trackedBranch ? trackedBranch->getShorthandName() : "-") << std::endl;

    auto remoteName = getUpstreamRemoteName(*head);
    rMessage() << head->getShorthandName() << " is set up to track remote " << remoteName << std::endl;

    auto remote = getRemote(remoteName);

    if (!remote)
    {
        rWarning() << "Cannot fetch from remote 'origin'" << std::endl;
        return;
    }

    remote->fetch();
}

git_repository* Repository::_get()
{
    return _repository;
}

}

}
