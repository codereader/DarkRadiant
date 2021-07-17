#pragma once

#include <git2.h>
#include "GitArchiveTextFile.h"
#include "GitException.h"

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

    GitArchiveTextFile::Ptr openTextFile(const std::string& filePath, Repository& repository)
    {
        // Tree entry is owned by the tree, it's not allowed to free it
        auto entry = git_tree_entry_byname(_tree, filePath.c_str());

        if (!entry) throw GitException("File not found: " + filePath);

        git_blob* blob;
        git_blob_lookup(&blob, repository._get(), git_tree_entry_id(entry));

        // GitArchiveTextFile assumes ownership of the blob
        return std::make_shared<GitArchiveTextFile>(blob, filePath);
    }

    git_tree* _get() const
    {
        return _tree;
    }
};

}

}
