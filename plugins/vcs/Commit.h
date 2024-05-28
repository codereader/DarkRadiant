#pragma once

#include "Repository.h"
#include "GitException.h"
#include "Tree.h"
#include <git2.h>

namespace vcs
{

namespace git
{

class Commit final
{
private:
    git_commit* _commit;

public:
    using Ptr = std::shared_ptr<Commit>;

    Commit(git_commit* commit) :
        _commit(commit)
    {}

    ~Commit()
    {
        git_commit_free(_commit);
    }

    const git_oid* getOid() const
    {
        return git_commit_id(_commit);
    }

    std::shared_ptr<Tree> getTree()
    {
        git_tree* tree;
        auto error = git_commit_tree(&tree, _commit);
        GitException::ThrowOnError(error);

        return std::make_shared<Tree>(tree);
    }

    static Ptr LookupFromOid(git_repository* repository, git_oid* oid)
    {
        git_commit* commit;

        auto error = git_commit_lookup(&commit, repository, oid);
        GitException::ThrowOnError(error);

        return std::make_shared<Commit>(commit);
    }

    git_commit* _get()
    {
        return _commit;
    }
};

}

}
