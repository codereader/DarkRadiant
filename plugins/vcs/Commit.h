#pragma once

#include "Repository.h"
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

    std::shared_ptr<Tree> getTree()
    {
        git_tree* tree;
        git_commit_tree(&tree, _commit);

        return std::make_shared<Tree>(tree);
    }
};

}

}
