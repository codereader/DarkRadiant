#pragma once

#include <git2.h>

namespace vcs
{

namespace git
{

class Tree final
{
private:
    git_tree* _tree;

public:
    Tree(git_tree* tree) :
        _tree(tree)
    {}

    ~Tree()
    {
        git_tree_free(_tree);
    }

    git_tree* _get() const
    {
        return _tree;
    }
};

}

}
