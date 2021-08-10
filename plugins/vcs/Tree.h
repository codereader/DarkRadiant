#pragma once

#include <git2.h>
#include "GitArchiveTextFile.h"
#include "GitException.h"
#include "Repository.h"

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
        git_tree_entry* entry = nullptr;
        auto error = git_tree_entry_bypath(&entry, _tree, filePath.c_str());
        GitException::ThrowOnError(error);

        git_blob* blob;
        error = git_blob_lookup(&blob, repository._get(), git_tree_entry_id(entry));

        // Free the entry before checking for errors
        git_tree_entry_free(entry);

        if (error < 0)
        {
            GitException::ThrowOnError(error);
        }

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
