#pragma once

#include <string>
#include <memory>
#include <git2.h>

namespace vcs
{

namespace git
{

struct RefSyncStatus
{
    RefSyncStatus() :
        localCommitsAhead(0),
        remoteCommitsAhead(0),
        localCanBePushed(true)
    {}

    // The number of commits the local branch is ahead of the remote
    std::size_t localCommitsAhead;

    // The number of commits the remote branch is ahead of the local
    std::size_t remoteCommitsAhead;

    // whether the local branch can be pushed on top of the remote
    bool localCanBePushed;

    // whether the local branch is up to date with the remote
    bool localIsUpToDate;
};

class Reference final
{
private:
    git_reference* _reference;

public:
    using Ptr = std::shared_ptr<Reference>;

    Reference(git_reference* reference) :
        _reference(reference)
    {}

    std::string getName() const
    {
        return git_reference_name(_reference);
    }

    std::string getShorthandName() const
    {
        return git_reference_shorthand(_reference);
    }

    // Returns the upstream of this reference (if configured)
    Ptr getUpstream() const
    {
        git_reference* upstream = nullptr;
        git_branch_upstream(&upstream, _reference);

        return upstream != nullptr ? std::make_shared<Reference>(upstream) : Ptr();
    }

    ~Reference()
    {
        git_reference_free(_reference);
    }

    static std::string OidToString(git_oid* oid)
    {
        std::string hexOid(GIT_OID_HEXSZ + 1, '\0');
        git_oid_fmt(hexOid.data(), oid);

        return hexOid;
    }
};

}

}
