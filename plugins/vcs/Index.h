#pragma once

#include <memory>
#include <string>
#include <git2.h>

namespace vcs
{

namespace git
{

class Tree;
class Repository;

class Index final
{
private:
    git_index* _index;

public:
    using Ptr = std::shared_ptr<Index>;
    
    Index(git_index* index);

    ~Index();

    // Corresponds to the operation done by "git add -A"
    void addAll();

    // Update all index entries to match the working directory
    void updateAll();

    // libgit2: Write an existing index object from memory back to disk using an atomic file lock.
    void write();

    // Collects this index and writes this to a tree, such that it can be used for a commit
    std::shared_ptr<Tree> writeTree(Repository& repository);

    bool hasConflicts();

    bool fileIsConflicted(const std::string& relativePath);

    void resolveByUsingOurs(const std::string& relativePath);
};

}

}
